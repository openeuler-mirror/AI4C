void set_points(float* dst, int* src, const int* divs, int divCount, int srcFixed,
                       int srcScalable, int srcStart, int srcEnd, float dstStart, float dstEnd,
                       bool isScalable) {
    float dstLen = dstEnd - dstStart;
    float scale;
    if (srcFixed <= dstLen) {
        // This is the "normal" case, where we scale the "scalable" patches and leave
        // the other patches fixed.
        scale = (dstLen - ((float) srcFixed)) / ((float) srcScalable);
    } else {
        // In this case, we eliminate the "scalable" patches and scale the "fixed" patches.
        scale = dstLen / ((float) srcFixed);
    }

    src[0] = srcStart;
    dst[0] = dstStart;
    for (int i = 0; i < divCount; i++) {
        src[i + 1] = divs[i];
        int srcDelta = src[i + 1] - src[i];
        float dstDelta;
        if (srcFixed <= dstLen) {
            dstDelta = isScalable ? scale * srcDelta : srcDelta;
        } else {
            dstDelta = isScalable ? 0.0f : scale * srcDelta;
        }
        dst[i + 1] = dst[i] + dstDelta;

        // Alternate between "scalable" and "fixed" patches.
        isScalable = !isScalable;
    }

    src[divCount + 1] = srcEnd;
    dst[divCount + 1] = dstEnd;
}