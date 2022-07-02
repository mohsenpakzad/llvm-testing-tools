/*
int myAbs(int x) {
  if (x > 0) {
    return x;
  } else {
    return x; // bug: should be ’-x’
  }
}
*/

int main() {
  int a1;
  int x = a1;
  if (x > 0) {
    return x;
  } else {
    return x; // bug: should be ’-x’
  }
}
