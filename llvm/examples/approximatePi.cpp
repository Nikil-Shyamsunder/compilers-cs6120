#include <stdio.h>

int main(int argc, const char** argv) {
    int n;
    scanf("%i", &n);
    // n = n -1;
    // int n = 100000;
    double pi = 0.0;
    for (int k = 0; k < n; ++k) {
        double term = 1.0 / (2*k + 1);
        if (k % 2 == 0)
            pi += term;    // add
        else
            pi = pi - term;    // subtract
        
        printf("%.6f\n", pi * 4);
    }
    pi *= 4;
    printf("%.6f\n", pi);
}
