#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#ifndef BLUR_H
#define BLUR_H

char* readKernelFile(char *faname);
int maskCreator(unsigned char *mask, int size);
int blurer(char* imageIn, char* imageOut, int maskDim);
void sequentialBlurer(char* pathIn, int maskDimention, char* pathOut);
cl_double clut_get_duration(cl_event perf_event);
double clut_get_real_time();

#endif 