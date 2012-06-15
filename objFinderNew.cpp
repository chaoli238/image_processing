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

#include "ObjFinderNew.h"
#include "ObjectData.h"
#include "Definitions.h"
#include <tbb/parallel_for.h>

#define MAX_OBJECTS 65534 //max objects per frame -- cannot be larger than max value of data type used
#define MAX_OBJSIZE 36 //maximum size of an object in pixels; objects larger than this will not be reported

void ObjFinderNew::findObjects( Image *image,
          unsigned int *num_objs,
          vector<unsigned short> &obj_xcol,
          vector<unsigned short> &obj_yrow )
{

	short unsigned int *raw_image = image->m_pixels;

    int i, j;
    int count, count2, count3, count4, count5;
    int flag1, flag2, N1, N2;
    short unsigned int* IP;
    int* IP2;
    int CUTOFF;

    int img_mean, img_stdev;

    unsigned short int yrows=IMG_HEIGHT, xcols=IMG_WIDTH;

    multimap <int, int> &m1 = *(new multimap<int, int>);
    map <int, int> &m2 = *(new map <int, int>);
    map <int, int> &m3 = *(new map <int, int>);
    map <int, int> &m4 = *(new map <int, int>);

    int *obj_size;
    int *centroid_xcol;
    int *centroid_yrow;
    short unsigned int return_numobjs, return_index;

    IP = (short unsigned int*) raw_image;

    char log_string[255];

    count = 0;

    // START BY COMPUTING IMAGE MEAN AND STDEV
    img_mean = mean(raw_image);
    img_stdev = stdev2(raw_image);

    // NOW DETERMINE PIXEL VALUE THRESHOLD TO APPLY
    // USES EMPIRICAL STDEV MULTIPLIER IN PROCESSORPARAMS

    CUTOFF = (int)((double)(img_stdev)) + img_mean;

    #ifdef DEBUG1
    sprintf(log_string, "find_objects: mean %d, std2 %d, threshold %d", img_mean, img_stdev, CUTOFF);
    p_log_simple(log_string);
    #endif


    if( (obj_size=(int*)malloc(MAX_BEADS_PERFRAME*sizeof(int))) == NULL){
        perror(NULL);
        fprintf(stderr, "ERROR allocating memory for obj_size\n");
    }
    if( (centroid_xcol=(int*)malloc(MAX_BEADS_PERFRAME*sizeof(int))) == NULL){
        perror(NULL);
        fprintf(stderr, "ERROR allocating memory for centroid_xcol\n");
    }
    if( (centroid_yrow=(int*)malloc(MAX_BEADS_PERFRAME*sizeof(int))) == NULL){
        perror(NULL);
        fprintf(stderr, "ERROR allocating memory for centroid_yrow\n");
    }

    if((IP2 = (int*) malloc(IMG_WIDTH*IMG_HEIGHT*sizeof(int))) == NULL){
        perror(NULL);
        fprintf(stderr, "Could not allocate enough memory for IP2.");
        exit(42);
    }

    count3=0;
    count2=1;  // store label count

    for(count = 0; count < (yrows * xcols); count++)
    { // 1-D index into image

        //Initialize centroid arrays for later
        if(count<MAX_BEADS_PERFRAME){
            *(obj_size + count) = 0;
            *(centroid_xcol + count) = 0;
            *(centroid_yrow + count) = 0;
        }

        IP2[count] = 0; // set object image value to 0

        //BEADS ARE LIGHT
        if (IP[count] > CUTOFF) { // current pixel is above threshold

        //BEADS ARE DARK
//        if ((IP[count] < CUTOFF)&&(img_stdev>45)) { // current pixel is below threshold     &&(img_mean<14100)

            flag1 = 0; flag2 = 0;
            N1 = count - xcols;
            N2 = count - 1;

			// Test upper pixel
            if ((N1 >= 0) && (IP2[N1] > 0) && (IP[N1] > CUTOFF)) {flag1 = 1;}

			// Test left pixel
            if ((count % xcols != 0) && (IP2[N2]) > 0 && (IP[N2] > CUTOFF)) {flag2 = 1;}

            if (flag1 == 0 && flag2 == 0)
			{
				IP2[count] = count2;
				count2++;
		    }  // untouched by labeled already-seen neighbors
            else if (flag1 == 1 && flag2 == 0)
			{
				IP2[count] = IP2[N1];
			}      // connected to pixel to top only
            else if (flag1 == 0 && flag2 == 1)
			{
				IP2[count] = IP2[N2];
			}      // connected to pixel to left only
            else
		    {      // connected to both neighbors
                IP2[count] = IP2[N1];

                if (IP2[N1] != IP2[N2]) {   // add entry (IP2[N1]=IP2[N2]) to equivalency table
                    m1.insert(pair<int, int>(IP2[N1],IP2[N2]));
                    m1.insert(pair<int, int>(IP2[N2],IP2[N1]));
                    m1.insert(pair<int, int>(IP2[N1],IP2[N1]));
                    m1.insert(pair<int, int>(IP2[N2],IP2[N2]));
                }
            }

            if (count2 > MAX_OBJECTS) {
            fprintf(stderr, "Too many objects in an image to keep track...\n");
            exit(42);
            }
        }
    } // end for loop

    // consolidate equivalency list

    for (count4 = 1; count4 < count2; count4++) {
        assign(count4, count4, m1, m2);
    }

    // run through image again and reassign labels

    map<int,int>::iterator p;
    map<int,int>::iterator p2;

    count4 = 1;

    for (count5 = 1; count5 < count2; count5++) {
        if (m2.count(count5) == 0) {
            m2.insert(pair<int, int>(count5, -1));
        }
        p = m2.find(count5);
        if (m3.count(p->second) > 0 && p->second != -1) {
            p2 = m3.find(p->second);
            m4.insert(pair<int,int>(count5,p2->second));
        }
        else {
            m4.insert(pair<int,int>(count5,count4));
            m3.insert(pair<int,int>(p->second,count4));
            count4++;
        }
    } // end for

    count5 = 0;

    for (count4 = 1; count4 < count2; count4++) {
        p = m4.find(count4);
        if (p->second > count5) {count5 = p->second;}
    }

    // Iterate over the bwlabel image, replacing values with object numbers specified by the m4 map
    // Iterate over bwlabel image, computing centroids for each object
    for(count = 0; count < (yrows * xcols); count++)
	{
        if (IP2[count]>0) 
		{
          p = m4.find(IP2[count]);
          IP2[count] = p->second;
          *(obj_size + p->second) = *(obj_size + p->second) + 1;
          *(centroid_xcol + IP2[count]) += (count % xcols);
          *(centroid_yrow + IP2[count]) += (int) floor((float)count/(float)xcols);
        }
    } // end for
    
	if(count5 > MAX_OBJECTS){
        sprintf(log_string, "WARNING: too many objects in the image; only returning first %d", MAX_OBJECTS);
    }

    *num_objs = count5;
    return_index=0;
    return_numobjs=0;
    if(*num_objs > MAX_OBJECTS)
    {
        *num_objs = MAX_OBJECTS;
    }

    for(i=1; i<*num_objs+1; i++)
    {
        // DONT REPORT THIS OBJECT IF IT IS TOO BIG
        if( *(obj_size+i) <= MAX_OBJSIZE )
        {
        	unsigned short output_x = (int)ceil((double)*(centroid_xcol + i) / (double)*(obj_size + i)); // assign return args
        	unsigned short output_y = (int)ceil((double)*(centroid_yrow + i) / (double)*(obj_size + i));

            obj_xcol.push_back(output_x);
            obj_yrow.push_back(output_y);
        }

    } // end for i

    *num_objs = obj_xcol.size();

 //   printf("   %d objects found, only use %d (which are not bigger than the defined size).\n", count5, return_numobjs);

    //free memory
    free(IP2);
    free(obj_size);
    free(centroid_xcol);
    free(centroid_yrow);

    delete &m1;
    delete &m2;
    delete &m3;
    delete &m4;
}



int ObjFinderNew::assign(int A, int B, multimap<int,int>&m1, map<int,int> &m2)
{

    multimap<int,int>::iterator iter;
    multimap<int,int>::iterator lower;
    multimap<int,int>::iterator upper;

    lower = m1.lower_bound(A);
    upper = m1.upper_bound(A);

    for (iter = lower; iter != upper; iter++) {

        if (m2.count(iter->second) == 0) {
              m2.insert(pair<int, int>(iter->second,B));
              assign(iter->second,B, m1, m2);
        }
    }

  return 0;
}



int ObjFinderNew::stdev(short unsigned int *data)
{
    int i;
    int img_mean;
    long long int sigma=0;
    double std=0;

    img_mean = mean(data);

    for(i=0; i<IMG_WIDTH*IMG_HEIGHT; i++)
	{
        //    fprintf(stdout, "%f\n", sqrt(pow(*(data+i)-img_mean,2)));
        sigma += (long long int)(pow((double)(*(data+i)-img_mean),2));
    }

    std = sqrt((1.0/((double)(IMG_WIDTH*IMG_HEIGHT)-1)) * sigma);
    return (int)std;
}

int ObjFinderNew::stdev2(short unsigned int *data)
{
    long long int std[IMG_HEIGHT];
    long long int stdsum;
    long long int imgsum;
    int img_mean;
    int std_mean;
    long long int sigma;
    double std2;

    int i,j;

    stdsum = 0;

    for(i=0; i<IMG_HEIGHT; i++)
	{
        sigma = 0;
        imgsum = 0;

        for(j=0; j<IMG_WIDTH; j++)
		{
            imgsum += (long long int)(*(data+(IMG_WIDTH*i)+j));
        }
        
		img_mean = imgsum / IMG_WIDTH;

        for(j=0; j<IMG_WIDTH; j++)
		{
            sigma += (long long int)(pow((double)(*(data+(IMG_WIDTH*i)+j))-img_mean, 2));
        }

        std[i] = (long long int)(sqrt((1.0/((double)(IMG_WIDTH-1))) * sigma));
        stdsum += std[i];
    } // end for i

    std_mean = (int)(stdsum / IMG_HEIGHT);

    sigma = 0;
    for(i=0; i<IMG_HEIGHT; i++)
	{
        sigma += (long long int)(pow((double)(std[i]-std_mean),2));
    }

    std2 = sqrt((1.0/((double)(IMG_HEIGHT)-1)) * sigma);
    
	return (int)std2;
}


int ObjFinderNew::mean(short unsigned int *data){
    double sum=0;

    int i;
    int mean;

    for(i=0; i<IMG_WIDTH*IMG_HEIGHT; i++){
        sum+=*(data+i);
    }
    //fprintf(stdout, "sum %lld, %d\n", sum, sum);
    mean = (int) ((double)sum / (double)(IMG_WIDTH*IMG_HEIGHT));
    return mean;
}

