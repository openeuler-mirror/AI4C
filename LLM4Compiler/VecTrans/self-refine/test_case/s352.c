float s352(int iterations,int LEN_1D, float* a, float *b)
{
    float dot;
    for (int nl = 0; nl < 8*iterations; nl++) {
        dot = (float)0.;
        for (int i = 0; i < LEN_1D; i += 5) {
            dot = dot + a[i] * b[i] + a[i + 1] * b[i + 1] + a[i + 2]
                * b[i + 2] + a[i + 3] * b[i + 3] + a[i + 4] * b[i + 4];
        }
    }
    return dot;
}