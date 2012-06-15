/******************************************************************************

Copyright 2011 Wyss Institute at Harvard University. All rights reserved.  

Authors: Chao Li

The MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.

******************************************************************************/
#include "baseCaller.h"
#include "ObjectData.h"
#include <tbb/parallel_for.h>
#include <math.h>
#include <float.h>
#include <windows.h>

#define threshold (64*64*4)
BaseCaller::BaseCaller(DataStorage *dataStorage) : m_dataStorage(dataStorage), filter(parallel)
{
}

BaseCaller::~BaseCaller()
{
}

void *BaseCaller::operator()( void* item )
{
	ImageSet *imageSet = static_cast<ImageSet*>(item);

	// Extract object values from ImageSet

	float clusterCentroid[16];
	int clusterPopulation[4] = { 0, 0, 0, 0 };

	memset( clusterCentroid, 0, 16 * sizeof( float ) );

	vector<float> normalizedValues;

	unsigned short *objVal = &imageSet->m_objectData->m_objectValue[0];

	// Calculate 4D vector means
	for( int i = 0 ; i < imageSet->m_objectData->m_objectValue.size() ; i += 4 )
	{
		float X = objVal[i+0];
		float Y = objVal[i+1];
		float Z = objVal[i+2];
		float W = objVal[i+3];

		normalizedValues.push_back( X );
		normalizedValues.push_back( Y );
		normalizedValues.push_back( Z );
		normalizedValues.push_back( W );

		int principalAxis = 0;
		float principalAxisMagnitude = X;

		if( Y > principalAxisMagnitude )
		{
			principalAxisMagnitude = Y;
			principalAxis = 1;
		}

		if( Z > principalAxisMagnitude )
		{
			principalAxisMagnitude = Z;
			principalAxis = 2;
		}

		if( W > principalAxisMagnitude )
		{
			principalAxisMagnitude = W;
			principalAxis = 3;
		}

		if( principalAxisMagnitude > 64 )
		{
			clusterCentroid[0+principalAxis*4] += X;
			clusterCentroid[1+principalAxis*4] += Y;
			clusterCentroid[2+principalAxis*4] += Z;
			clusterCentroid[3+principalAxis*4] += W;

			clusterPopulation[principalAxis]++;
		}
	}

	for( int i = 0 ; i < 4 ; ++i )
	{
		clusterCentroid[0+i*4] /= (float)clusterPopulation[i];
		clusterCentroid[1+i*4] /= (float)clusterPopulation[i];
		clusterCentroid[2+i*4] /= (float)clusterPopulation[i];
		clusterCentroid[3+i*4] /= (float)clusterPopulation[i];
	}	

	// Assign to clusters by euclidean distance to cluster center

	float *nObjVal = &normalizedValues[0];
	unsigned short outputValue = 0;
	int outputCounter = 0;

	for( int i = 0 ; i < imageSet->m_objectData->m_objectValue.size() ; i += 4 )
	{
		float X = nObjVal[i+0];
		float Y = nObjVal[i+1];
		float Z = nObjVal[i+2];
		float W = nObjVal[i+3];

		float minDistance = (clusterCentroid[0] - X) * (clusterCentroid[0] - X) +
		(clusterCentroid[1] - Y) * (clusterCentroid[1] - Y) +
		(clusterCentroid[2] - Z) * (clusterCentroid[2] - Z) +
		(clusterCentroid[3] - W) * (clusterCentroid[3] - W);

		int closestCluster = 0;

		for( int j = 1 ; j < 4 ; ++j )
		{
			float distance = (clusterCentroid[0+j*4] - X) * (clusterCentroid[0+j*4] - X) +
			(clusterCentroid[1+j*4] - Y) * (clusterCentroid[1+j*4] - Y) +
			(clusterCentroid[2+j*4] - Z) * (clusterCentroid[2+j*4] - Z) +
			(clusterCentroid[3+j*4] - W) * (clusterCentroid[3+j*4] - W);

			if( distance < minDistance )
			{
				minDistance = distance;
				closestCluster = j;
			}
		}

		float vectorMagnitude = (X*X) + (Y*Y) + (Z*Z) + (W*W);

		if( vectorMagnitude < (float)threshold )
		{
			closestCluster = 15;
		}

		if( i % 16 == 12 )
		{
			outputValue |= closestCluster;
			imageSet->m_objectData->m_objectValue[outputCounter++] = outputValue;
			outputValue = 0;
		}
		else
		{
			outputValue |= closestCluster;
			outputValue <<= 4;
		}
	}

	imageSet->m_objectData->m_objectValue.resize(outputCounter);

	return imageSet;
}
