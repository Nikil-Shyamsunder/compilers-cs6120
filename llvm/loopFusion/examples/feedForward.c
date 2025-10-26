#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N 1000
#define M 1000
#define ITERATIONS 100

// multiply vector a and vector b, store result in c and do ReLU
// split over two loop
void feedForward(float a[50], float b[50], float c[50]) {
    // int acc = 0;
    // for (int j = 0; j < 50; j++){
    //     acc += j;
    // }

    for (int i = 0; i < 50; i++) {
        c[i] = a[i] * b[i];
    }
    for (int i = 0; i < 50; i++) {
        // perform ReLU
        if (c[i] < 0) {
            c[i] = 0;
        }
    }
    return;
}

// Matrix-vector multiply: matrix[N][M] * vector[M] = result[N]
// Split into two loops that can be fused
void matVecMultiply(float matrix[N][M], float vector[M], float result[N]) {
    // Initialize result to zero
    for (int i = 0; i < N; i++) {
        result[i] = 0.0f;
    }

    // Compute matrix-vector product
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            result[i] += matrix[i][j] * vector[j];
        }
    }
}

int main(int argc, const char** argv){
    // Allocate memory for matrix and vectors
    float (*matrix)[M] = malloc(N * M * sizeof(float));
    float *vector = malloc(M * sizeof(float));
    float *result = malloc(N * sizeof(float));

    // Initialize matrix and vector with random values
    srand(42);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            matrix[i][j] = (float)rand() / RAND_MAX;
        }
    }
    for (int i = 0; i < M; i++) {
        vector[i] = (float)rand() / RAND_MAX;
    }

    // Benchmark: run multiple iterations
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int iter = 0; iter < ITERATIONS; iter++) {
        matVecMultiply(matrix, vector, result);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate elapsed time in microseconds
    long long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000LL +
                           (end.tv_nsec - start.tv_nsec) / 1000;

    // Print timing result (Python script will parse this)
    printf("Elapsed time: %lld microseconds\n", elapsed_us);
    printf("Average per iteration: %.2f microseconds\n", elapsed_us / (double)ITERATIONS);

    // Print a sample result to prevent optimization elimination
    printf("Sample result[0]: %.6f\n", result[0]);

    // Cleanup
    free(matrix);
    free(vector);
    free(result);

    return 0;
}