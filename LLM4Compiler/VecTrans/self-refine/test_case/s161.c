void s161(int iterations, int LEN_1D, float* a, float* b,float*c, float*d,float*e)
{
    for (int nl = 0; nl < iterations/2; nl++) {
        for (int i = 0; i < LEN_1D-1; ++i) {
            if (b[i] < (float)0.) {
                goto L20;
            }
            a[i] = c[i] + d[i] * e[i];
            goto L10;
L20:
            c[i+1] = a[i] + d[i] * d[i];
L10:
            ;
        }
    }
}