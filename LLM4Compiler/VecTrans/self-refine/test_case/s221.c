void s221(int iterations, int LEN_1D, float*a,float*b,float*c,float*d)
{
    for (int nl = 0; nl < iterations/2; nl++) {
        for (int i = 1; i < LEN_1D; i++) {
            a[i] += c[i] * d[i];
            b[i] = b[i - 1] + a[i] + d[i];
        }
    }
}