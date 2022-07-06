int main() {
    int a1;
    a1 = a1 + 1;
    if (a1 != 5) {
        return a1;
    } else {
        return a1; // bug: should be ’-x’
    }
}