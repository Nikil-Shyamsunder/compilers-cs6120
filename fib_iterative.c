#include <stdio.h>

int fib(int n) {
    if (n <= 1) {
        return n;
    }
    int a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

int main() {
    int n;
    printf("Enter n: ");
    scanf("%d", &n);
    int result = fib(n);
    printf("fib(%d) = %d\n", n, result);
    return 0;
}
