void s291(int iterations,int LEN_1D, float* a,float* b)
{
    int im1;
    for (int nl = 0; nl < 2*iterations; nl++) {
        im1 = LEN_1D-1;
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = (b[i] + b[im1]) * (float).5;
            im1 = i;
        }
    }
}