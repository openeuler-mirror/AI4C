void s258(int iterations, float* a,float* b,float* c, float* d,float* e, float aa[256][256])
{
    float s;
    for (int nl = 0; nl < iterations; nl++) {
        s = 0.;
        for (int i = 0; i < 256; ++i) {
            if (a[i] > 0.) {
                s = d[i] * d[i];
            }
            b[i] = s * c[i] + d[i];
            e[i] = (s + (float)1.) * aa[0][i];
        }
    }
}