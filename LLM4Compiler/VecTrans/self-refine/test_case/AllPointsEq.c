bool AllPointsEq(const int pts[], int count) {
    for (int i = 1; i < count; ++i) {
        if (pts[0] != pts[i]) {
            return false;
        }
    }
    return true;
}