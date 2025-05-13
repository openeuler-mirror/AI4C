void s114(int iterations, float aa[256][256], float bb[256][256])
{
    for (int nl = 0; nl < 200*(iterations/(256)); nl++) {
        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < i; j++) {
                aa[i][j] = aa[j][i] + bb[i][j];
            }
        }
    }
}