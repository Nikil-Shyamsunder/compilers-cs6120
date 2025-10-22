// multiply A and vector b, store result in c and do ReLU
// split over two loop
void feedForward(int N, int M, float A[N][M], float b[M], float c[N]) {
    for (int i = 0; i < N; i++) {
        c[i] = 0;
        for (int j = 0; j < M; j++) {
            c[i] += A[i][j] * b[j];
        }
    }
    for (int i = 0; i < N; i++) {
        // perform ReLU
        if (c[i] < 0) {
            c[i] = 0;
        }
    }
    return;
}

int main(int argc, const char** argv){
    // run small example

    feedForward(2, 3,
        (float[2][3]){{1.0, -2.0, 3.0},
                      {-4.0, 5.0, -6.0}},
        (float[3]){0.5, -1.0, 1.5},
        (float[2]){0.0, 0.0});
}