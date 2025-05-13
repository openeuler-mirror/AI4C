void s2251(int iterations, int LEN_1D, float* a,float* b,float* c,float* d, float* e)
{
    for (int nl = 0; nl < iterations; nl++) {
        float s = (float)0.0;
        for (int i = 0; i < LEN_1D; i++) {
            a[i] = s*e[i];
            s = b[i]+c[i];
            b[i] = a[i]+d[i];
        }
    }
}