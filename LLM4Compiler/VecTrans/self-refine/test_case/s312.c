float s312(int iterations,int LEN_1D, float* a)
{

    float prod;
    for (int nl = 0; nl < 10*iterations; nl++) {
        prod = (float)1.;
        for (int i = 0; i < LEN_1D; i++) {
            prod *= a[i];
        }
    }
    return prod;
}