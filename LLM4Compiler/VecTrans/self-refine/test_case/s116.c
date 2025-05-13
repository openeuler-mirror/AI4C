void s116(int iterations, int LEN_1D, float* a)
{
    for (int nl = 0; nl < iterations*10; nl++) {
        for (int i = 0; i < LEN_1D - 5; i += 5) {
            a[i] = a[i + 1] * a[i];
            a[i + 1] = a[i + 2] * a[i + 1];
            a[i + 2] = a[i + 3] * a[i + 2];
            a[i + 3] = a[i + 4] * a[i + 3];
            a[i + 4] = a[i + 5] * a[i + 4];
        }
    }
}