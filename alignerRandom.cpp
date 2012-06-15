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
#include "objectFinderRandom.h"
#include "Image.h"
#include "ObjectData.h"
#include <tbb/parallel_for.h>

#define GRANULARITY 3

objectFinderRandom::objectFinderRandom(DataStorage *dataStorage) : m_dataStorage(dataStorage), filter(parallel)
{
// TODO Auto-generated constructor stub

}

objectFinderRandom::~objectFinderRandom() {
// TODO Auto-generated destructor stub
}

void *objectFinderRandom::operator()( void* item )
{

	Entity *entity = static_cast<Entity*>(item);

	// If the first cycle has terminated, pass on the image data to the alignment phase, otherwise the image data has already been extracted
	if( entity->m_cycle == 0 )
	{
		// printf("ImageAligner: forwarded\n");
		return item;
	}

	ImageSet *imageSet = static_cast<ImageSet*>(item);

	// Image alignment
	int offsetX = 0;
	int offsetY = 0;

	ImageSet *referenceImageSet = m_dataStorage->m_imageSets[imageSet->m_id];

	printf("objectFinderRandom: working %d\n", imageSet->m_id);

	alignImage( referenceImageSet, imageSet, referenceImageSet->m_alignmentData, offsetX, offsetY );

	// Extract data using generated offsets

	ObjectData *referenceObjectData = m_dataStorage->m_imageSets[imageSet->m_id]->m_objectData;
	ObjectData *objectData = new ObjectData();

	objectData->m_objectValue.resize(referenceObjectData->m_numObjects);

	imageSet->m_objectData = objectData;
	objectData->m_numObjects = referenceObjectData->m_numObjects;

	// Extract values

	for( int j = 0 ; j < referenceObjectData->m_numObjects ; ++j )
	{
		if( referenceObjectData->m_objectPosX[j] + offsetX < 0 ||
			referenceObjectData->m_objectPosX[j] + offsetX >= IMG_WIDTH ||
			referenceObjectData->m_objectPosY[j] + offsetY < 0 ||
			referenceObjectData->m_objectPosY[j] + offsetY >= IMG_HEIGHT )
		{
			objectData->m_objectValue.push_back( 0 );
			objectData->m_objectValue.push_back( 0 );
			objectData->m_objectValue.push_back( 0 );
			objectData->m_objectValue.push_back( 0 );
		}
		else
		{
			objectData->m_objectValue.push_back( imageSet->m_images[0]->Pixel( referenceObjectData->m_objectPosX[j] + offsetX, referenceObjectData->m_objectPosY[j] + offsetY ) );
			objectData->m_objectValue.push_back( imageSet->m_images[1]->Pixel( referenceObjectData->m_objectPosX[j] + offsetX, referenceObjectData->m_objectPosY[j] + offsetY ) );
			objectData->m_objectValue.push_back( imageSet->m_images[2]->Pixel( referenceObjectData->m_objectPosX[j] + offsetX, referenceObjectData->m_objectPosY[j] + offsetY ) );
			objectData->m_objectValue.push_back( imageSet->m_images[3]->Pixel( referenceObjectData->m_objectPosX[j] + offsetX, referenceObjectData->m_objectPosY[j] + offsetY ) );
		}
	}

	// printf("%06d/%06d: Extracted %d objects\n", imageSet->m_id, ARRAYS_PER_FC * IMGS_PER_ARRAY, referenceObjectData->m_numObjects);

	for( int i = 0 ; i < 4 ; ++i )
	{
		// Delete full image
		delete imageSet->m_images[i];
		imageSet->m_images[i] = 0;
	}

	return imageSet;
	
}

void objectFinderRandom::alignImage(ImageSet *reference, ImageSet *toAlign, AlignmentData *alignmentData, int &offsetX, int &offsetY)
{
	int threshold = 0;
	int sOffsetX = 0;
	int sOffsetY = 0;

	unsigned short *vectorX = &alignmentData->m_alignmentOffsetX[0];
	unsigned short *vectorY = &alignmentData->m_alignmentOffsetY[0];
	unsigned short *image1 = toAlign->m_images[0]->m_pixels;
	unsigned short *image2 = toAlign->m_images[1]->m_pixels;
	unsigned short *image3 = toAlign->m_images[2]->m_pixels;
	unsigned short *image4 = toAlign->m_images[3]->m_pixels;

	// Align with individual objects coordinates, 30x30 pixels window
	int cumulative = 0;

	// Coarse alignment first
	for( int y = -ALIGNMENT_WINDOW_HEIGHT ; y <= ALIGNMENT_WINDOW_HEIGHT ; y += GRANULARITY )
	{
		for( int x = -ALIGNMENT_WINDOW_WIDTH ; x <= ALIGNMENT_WINDOW_WIDTH ; x += GRANULARITY )
		{
			for( int i = 0 ; i < ALIGNMENT_OBJECTS ; ++i )
			{
				cumulative += *(image1 + vectorX[i] + x + (vectorY[i] + y)*IMG_WIDTH);
			}
			for( int i = 0 ; i < ALIGNMENT_OBJECTS ; ++i )
			{
				cumulative += *(image2 + vectorX[i] + x + (vectorY[i] + y)*IMG_WIDTH);
			}
			for( int i = 0 ; i < ALIGNMENT_OBJECTS ; ++i )
			{
				cumulative += *(image3 + vectorX[i] + x + (vectorY[i] + y)*IMG_WIDTH);
			}
			for( int i = 0 ; i < ALIGNMENT_OBJECTS ; ++i )
			{
				cumulative += *(image4 + vectorX[i] + x + (vectorY[i] + y)*IMG_WIDTH);
			}

			cumulative /= 4;

			if( cumulative > threshold )
			{
				threshold = cumulative;
				sOffsetX = x;
				sOffsetY = y;
			}

			cumulative = 0;
		}
	}

	offsetX = sOffsetX;
	offsetY = sOffsetY;

}
