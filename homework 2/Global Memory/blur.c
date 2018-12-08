#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "pgm.h"
#include "pgm.c"
#include "blur.h"

#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif

#define LOCAL_S 16
  
int main(int argc, char *argv[])
{	
    if (argc != 4) 
    {
        printf("Error: wrong number of arguments:\n");
        printf("- arg1 [char*]: path of the input image\n");
        printf("- arg2 [char*]: path of the output image\n");
        printf("- arg3 [int]: blurring value, an odd number\n");
    }
    else 
    {
        FILE* file1 = fopen(argv[1], "r");
        FILE* file2 = fopen(argv[2], "w");

        if (file1 == NULL)      { printf("Error: could not read the input image, unexisting path.\n");   exit(1); } 
        else if (file2 == NULL) { printf("Error: could not store the output image, unexisting path.\n"); exit(1); }  
        else if (atoi(argv[3])%2 == 0) { printf("Error: the dimention of the mask must be an odd number.\n");   exit(1); }
        
        // Chiama la funzione che rende sfuocata l'immagine in input
        else blurer(argv[1], argv[2], atoi(argv[3])); 
    }
}

int blurer(char* pathIn, char* pathOut, int maskDimention)
{   
    int imageCenter = maskDimention/2;

    // Pronta la matrice contenente i pixel dell'immagine in input.
    unsigned char *imagein;
    
    // Pronti i puntatori ai valori dell'altezza e della larghezza dell'immagine in input
    int height, width;

    // Carico l'immagine
    pgm_load(&imagein, &height, &width, pathIn);

    // Calcola le dimensioni della nuova immagine
    int outHeight = height - imageCenter*2;
    int outWidth = width - imageCenter*2;
    
    int dimOut = outHeight*outWidth;
    int dimIn = height*width;

    if (maskDimention > height || maskDimention > width) 
    {
        printf("Error: the dimention of the mask is greater that the dimention of the input image.\n");
        exit(1);
    }

    // Crea la matrice mask
    unsigned char *mask;
    mask = malloc(maskDimention*maskDimention*sizeof(unsigned char));
    int ones = maskCreator(mask, maskDimention);
    
    // Crea il vettore contenente l'immagine elaborata
    unsigned char *imageout;
    imageout = malloc(dimOut*sizeof(unsigned char));

    printf("\nLe dimensioni dell'immagine in output è %i (%ix%i), prima era %i (%ix%i).\n", dimOut, outWidth, outHeight, dimIn, width, height);

    //////------------------OPEN_CL-------------------////// 

    int err = 0;                 
    cl_kernel kernel;                  
    cl_event  event;                   
    cl_platform_id platform_id;
    cl_device_id device_id;             
    cl_context context;                
    cl_command_queue commands;         
    cl_program program;                 

    err = clGetPlatformIDs(1, &platform_id, NULL);
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_GPU, 1, &device_id, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to create a device group!\n");
        return EXIT_FAILURE;
    }

    context = clCreateContext(0, 1, &device_id, NULL, NULL, &err);
    if (!context)
    {
        printf("Error: Failed to create a compute context!\n");
        return EXIT_FAILURE;
    }

    commands = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, &err);
    if (!commands)
    {
        printf("Error: Failed to create a command commands!\n");
        return EXIT_FAILURE;
    }

    // Crea i buffer per i devices 
    cl_mem a_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, dimIn*sizeof(unsigned char), imagein, NULL);
    cl_mem b_mem_obj = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, maskDimention*maskDimention*sizeof(unsigned char), mask, NULL);
    cl_mem c_mem_obj = clCreateBuffer(context, CL_MEM_WRITE_ONLY, dimOut*sizeof(unsigned char), NULL, NULL);
    if (!a_mem_obj || !b_mem_obj || !c_mem_obj)
    {
        printf("Error: Failed to allocate device memory!\n");
        exit(1);
    }  

    // Crea il programma e il Kernel
    char *programmaKernel = readKernelFile("pictureProc.cl");

    program = clCreateProgramWithSource(context, 1, (const char **) & programmaKernel, NULL, &err);
    if (err != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];

        printf("Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        exit(1);
    }

    err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "pictureProc", &err);
    if (!kernel || err != CL_SUCCESS)
    {
        printf("Error: Failed to create compute kernel or program!\n");
        exit(1);
    }

    // Imposta gli argomenti del Kernel
    err  = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem_obj);    
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem_obj);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&c_mem_obj);
    err |= clSetKernelArg(kernel, 3, sizeof(int), (void *)&maskDimention);
    err |= clSetKernelArg(kernel, 4, sizeof(int), (void *)&width);
    err |= clSetKernelArg(kernel, 5, sizeof(int), (void *)&outWidth);
    err |= clSetKernelArg(kernel, 6, sizeof(int), (void *)&outHeight);
    err |= clSetKernelArg(kernel, 7, sizeof(int), (void *)&ones);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to set kernel arguments! %d\n", err);
        exit(1);
    }

    // Imposta il numero di work-groups e la loro dimensione, global_item_size sarà un multiplo di local_item_size
    size_t global_item_size[2] =  { ((width+LOCAL_S-1)/LOCAL_S)*LOCAL_S, ((height+LOCAL_S-1)/LOCAL_S)*LOCAL_S };
    size_t local_item_size[2] = { LOCAL_S, LOCAL_S };
    
    printf("Esecuzione su memoria GLOBALE:\n\n");

    err = clEnqueueNDRangeKernel(commands, kernel, 2, NULL, global_item_size, local_item_size, 0, NULL, &event);
    if (err)
    {
        printf("Error: Failed to execute kernel!\n");
        return EXIT_FAILURE;
    }

    printf("Dimensione della memoria globale: %zu x %zu\n", global_item_size[0], global_item_size[1]);
    printf("Dimensione della memoria locale: %zu x %zu\n\n", local_item_size[0], local_item_size[1]);

    // Attende il completamento della computazione e aggiorna il contenuto del vettore imageout
    clFinish(commands);
    err = clEnqueueReadBuffer(commands, c_mem_obj, CL_TRUE, 0, dimOut *sizeof(unsigned char), imageout, 0, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to read output array! %d\n", err);
        exit(1);
    }

    printf("Tempo esecuzione su GPU: %f sec\n", clut_get_duration(event));

    //Salva la nuova immagine elaborata
    pgm_save(imageout, outHeight, outWidth, pathOut);

    //////-------- INIZIO ESECUZIONE SEQUENZIALE ----------//////

    double start = clut_get_real_time();

    int correct = 1;
    for (int h = 0; h < outHeight; h++)
    {
        for (int l = 0; l < outWidth; l++)
        {
            int sum = 0;
            for (int r = 0; r < maskDimention; r++)
            {
                for (int c = 0; c < maskDimention; c++)
                {
                    int x = c+l;
                    int y = r+h;                    
                    sum += imagein[width*y+x]*mask[maskDimention*r+c];
                }
            }
            //printf("%i -> %i\n", imageout[outWidth*h+l], sum/ones);
            if (imageout[outWidth*h+l] != sum/ones) { correct = 0; break; }
        }
    }
    double stop = clut_get_real_time();

    printf("Tempo esecuzione su CPU: %f sec\n", stop-start);

    if (correct) printf("Test di correttezza: SUPERATO\n\n");
    else         printf("Test di correttezza: FALLITO\n\n");

    //////-------- FINE ESECUZIONE SEQUENZIALE ----------//////

    // Libera le risorse utilizzate
    clReleaseMemObject(a_mem_obj);
    clReleaseMemObject(b_mem_obj);
    clReleaseMemObject(c_mem_obj);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);
    free(mask);
    free(imagein);
    free(imageout);

    return 0;
}

// credits to Camil Demetrescu.
cl_double clut_get_duration(cl_event perf_event) 
{
    cl_ulong start = 0, end = 0;

    clGetEventProfilingInfo(perf_event, 
                            CL_PROFILING_COMMAND_START, 
                            sizeof(cl_ulong), &start, NULL);

    clGetEventProfilingInfo(perf_event, 
                            CL_PROFILING_COMMAND_END, 
                            sizeof(cl_ulong), &end, NULL);

    return (cl_double)(end - start)*(cl_double)(1e-09);
}

// credits to Camil Demetrescu.
double clut_get_real_time() 
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec+tv.tv_usec*1E-06;
}

// Legge il contenuto del file .cl
char* readKernelFile(char *faname)
{
    char*   source_str;
    size_t  source_size;
    FILE* fp = fopen(faname, "r");

    source_str  = malloc(65536);
    
    fread(source_str, 1, 65536, fp);
    fclose(fp);

    return source_str;
}

// Crea la mask e ritorna il numero di uno presenti nella mask
int maskCreator(unsigned char *mask, int size) 
{
    int half = size/2;
    int ones = 0;

    for (int k = 0; k < size; k++)
    {
        for (int q = 0; q < size; q++)
        {
            if ((k <= half && q >= half-k && q <=half+k) || (k > half && q > half-(size-k) && q <half+(size-k))) { mask[size*k+q] = 1; ones++; }
            else { mask[size*k+q] = 0;  }
        } 
    }
    return ones;
}