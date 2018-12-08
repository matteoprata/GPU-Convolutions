__kernel void pictureProc(__global unsigned char *A,
                          __global unsigned char *B,
                          __global unsigned char *C,
                          int maskDimention, int width, int outWidth, int outHeight, int ones)
{
    // Associa ad i e j gli indici globali, rispettivamente la larghezza e l'altezza dell'immagine di output
    int i = get_global_id(0);
    int j = get_global_id(1);

    // Ignora gli indici che superano la dimensione dell'immagine di output. 
    if (i >= outWidth || j >= outHeight) return;
    
    // Parte lo spostamento della matrice filtro su tutta l'immagine di input. 
    // In parallelo ogni work-group calcola l'intensità di un pixel dell'immagine di output.
    int sum = 0;
    for (int r = 0; r < maskDimention; r++)
    {
        for (int c = 0; c < maskDimention; c++)
        {
            int y = c+i;
            int x = r+j;
            sum += A[width*x+y] * B[maskDimention*r+c];
        }
    }
    // Salvataggio dell'immagine di output nel buffer C
    C[j*outWidth+i] = sum/ones;
}