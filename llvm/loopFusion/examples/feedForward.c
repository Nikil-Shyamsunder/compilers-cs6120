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

int main(int argc, const char** argv){
    // run small example

    // feedForward(3,
    //     (float[3]){1.0, -2.0, 3.0},
    //     (float[3]){0.5, -1.0, 1.5},
    //     (float[3]){0.0, 0.0, 0.0});
}