void s235(int iterations, float* a,float* b,float* c,float aa[256][256],float bb[256][256])
{
    for (int nl = 0; nl < 200*(iterations/256); nl++) {
        for (int i = 0; i < 256; i++) {
            a[i] += b[i] * c[i];
            for (int j = 1; j < 256; j++) {
                aa[j][i] = aa[j-1][i] + bb[j][i] * a[i];
            }
        }
    }
}