void s341(int iterations,int LEN_1D, float* a, float *b)
{
    int j;
    for (int nl = 0; nl < iterations; nl++) {
        j = -1;
        for (int i = 0; i < LEN_1D; i++) {
            if (b[i] > (float)0.) {
                j++;
                a[j] = b[i];
            }
        }
    }
}