int main() {
    int a1;
    if (a1 != 123456) {
        return a1;
    } else {
        return a1; // bug: should be ’-x’
    }
}