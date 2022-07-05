int main() {
    int a1;
    if (a1 != 90) {
        return a1;
    } else {
        return a1; // bug: should be ’-x’
    }
}