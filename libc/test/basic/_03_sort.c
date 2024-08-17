// RUN: rrisc32-cc -o %t.exe %s
// RUN: rrisc32-emulate %t.exe | filecheck %s

#include <stdio.h>

void print(int *arr, int n) {
  for (int i = 0; i < n; ++i)
    printf(i ? ", %d" : "%d", arr[i]);
  putl("");
}

void test(void (*sort)(int *, int)) {
  int arr[] = {1, -3, 2, 0, 5};
  int n = sizeof(arr) / sizeof(int);
  if (sort)
    sort(arr, n);
  print(arr, n);
}

void swap(int *p1, int *p2) {
  // !!!
  if (p1 == p2)
    return;

  *p1 ^= *p2;
  *p2 ^= *p1;
  *p1 ^= *p2;
}

void insertion_sort(int *arr, int n) {
  if (n < 2)
    return;
  for (int i = 1; i < n; ++i) {
    int j = i;
    while (j > 0 && arr[j] < arr[j - 1]) {
      swap(arr + j, arr + j - 1);
      --j;
    }
  }
}

void quick_sort(int *arr, int n) {
  if (n < 2)
    return;

  int pivot = arr[n - 1];
  int i = 0, j = 0;
  while (j < n) {
    if (arr[j] <= pivot) {
      swap(arr + i, arr + j);
      ++i;
    }
    ++j;
  }
  quick_sort(arr, i - 1);
  quick_sort(arr + i, n - i);
}

int main() {
  test(NULL);
  test(insertion_sort);
  test(quick_sort);
}

// CHECK:      1, -3, 2, 0, 5
// CHECK-NEXT: -3, 0, 1, 2, 5
// CHECK-NEXT: -3, 0, 1, 2, 5
