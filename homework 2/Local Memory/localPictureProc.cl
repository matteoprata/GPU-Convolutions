#define LOC 16

__kernel void pictureProc(__global unsigned char *A,
                          __global unsigned char *B,
                          __global unsigned char *C,
                          __local  unsigned char *LOCAL,
                          int localArrayDimention, int maskDimention, int width, 
                          int height, int outWidth, int outHeight, int ones)
{   
    int colonne = get_global_id(0); 
    int righe = get_global_id(1); 

    int lcolonne = get_local_id(0);
    int lrighe = get_local_id(1);

    // Assicura che gli indici globali sono contenuti nell'immagine di input. 
    if (colonne >= width || righe >= height) return; 

    // I vettori mantenuti localmente vengono riempiti con parte dell'immagine. 
    // In particolare, in questa fase la memoria LOCAL sarà piena di pixel contenuti in una pozione dell'immagine grande quanto LOC*LOC.
    LOCAL[lrighe*localArrayDimention+lcolonne] = A[righe*width+colonne];

    // Vanno aggiunti altri pixel al vettore LOCAL affinché riesca a computare il valore di pixel ai bordi del blocco.
    if (lcolonne == LOC-1)  
        for (int qq = 1; qq < maskDimention; qq++) LOCAL[lrighe * localArrayDimention + lcolonne+qq] = A[righe *width + colonne+qq]; 

    if (lrighe == LOC-1)    
        for (int qq = 1; qq < maskDimention; qq++) LOCAL[(lrighe+qq) * localArrayDimention + lcolonne] = A[(righe+qq) *width + colonne]; 

    if (lrighe == LOC-1 && lcolonne == LOC-1) 
    {
        for (int qq = 1; qq < maskDimention; qq++) 
        {
            for (int qz = 1; qz < maskDimention; qz++) 
                LOCAL[(lrighe+qq)* localArrayDimention + (lcolonne+qz)] = A[(righe+qq)*width + colonne+qz]; 
        }
    }

    // Aspetta che la memoria locale sia riempita completamente e poi inizia la computazone. 
    barrier(CLK_LOCAL_MEM_FENCE);
    
    if (colonne < outWidth && righe < outHeight) 
    {
        int sum = 0;
        for (int k = 0; k < maskDimention; k++)
        {
            for (int q = 0; q < maskDimention; q++)
            {
                int x = lcolonne + q;
                int y = lrighe + k;
                sum += LOCAL[y*localArrayDimention+x] * B[k*maskDimention+q];
            }
        }
        C[righe*outWidth+colonne] = sum/ones; 
    }
}