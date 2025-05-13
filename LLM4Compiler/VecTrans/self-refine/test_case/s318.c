float s318(int iterations,int LEN_1D, float* a, int inc)
{
    int k, index;
    float max, chksum;
    for (int nl = 0; nl < iterations/2; nl++) {
        k = 0;
        index = 0;
        max = fabsf(a[0]);
        k += inc;
        for (int i = 1; i < LEN_1D; i++) {
            if (fabsf(a[k]) <= max) {
                goto L5;
            }
            index = i;
            max = fabsf(a[k]);
L5:
            k += inc;
        }
        chksum = max + (float) index;
    }
    return max + index + 1;
}