float s3113(int iterations,int LEN_1D, float* a)
{
    float max;
    for (int nl = 0; nl < iterations*4; nl++) {
        max = fabsf(a[0]);
        for (int i = 0; i < LEN_1D; i++) {
            if ((fabsf(a[i])) > max) {
                max = fabsf(a[i]);
            }
        }
    }
    return max;
}