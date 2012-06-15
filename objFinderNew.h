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

#ifndef OBJECTFINDERNEW_H_
#define OBJECTFINDERNEW_H_

#include "DataStorage.h"
#include "Image.h"
#include "tbb/pipeline.h"
#include <vector>
#include <map>
#include <algorithm>
#include <iterator>
#include <math.h>

using namespace std;
using namespace tbb;

class ObjFinderNew: public filter
{
public:
	ObjFinderNew(DataStorage *dataStorage);
	virtual ~ObjFinderNew();

	void* operator()(void*);
	void findObjects(	Image *image,
						unsigned int *num_objs,
						vector<unsigned short> &obj_xcol,
						vector<unsigned short> &obj_yrow );

private:

	int assign(int A, int B, multimap<int,int>&m1, map<int,int> &m2);
	int stdev(short unsigned int *data);
	int stdev2(short unsigned int *data);
	int mean(short unsigned int *data);

	DataStorage *m_dataStorage;
};

#endif /* ObjFinderNew_H_ */
