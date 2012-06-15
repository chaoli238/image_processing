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

#ifndef baseCaller_H_
#define baseCaller_H_

#include "DataStorage.h"
#include "tbb/pipeline.h"
#include "fftw3.h"

using namespace tbb;

class baseCaller: public filter
{
public:
	baseCaller(DataStorage *dataStorage);
	virtual ~baseCaller();

	void* operator()(void*);

private:

	DataStorage *m_dataStorage;
};

#endif /* baseCaller_H_ */
