void s343(int iterations,float * flat_2d_array, float aa[256][256], float bb[256][256])
{
    int k;
    for (int nl = 0; nl < 10*(iterations/256); nl++) {
        k = -1;
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 256; j++) {
                if (bb[j][i] > (float)0.) {
                    k++;
                    flat_2d_array[k] = aa[j][i];
                }
            }
        }
    }
}