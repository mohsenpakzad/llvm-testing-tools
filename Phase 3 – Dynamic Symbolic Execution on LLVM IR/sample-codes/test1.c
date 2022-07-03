int main() {
    int a1;
    int x = a1;
    if (x != 123456) {
        return x;
    } else {
        return x; // bug: should be ’-x’
    }
}