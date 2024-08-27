#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

#define MAX 1000

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <dimension> <output file>\n", argv[0]);
        return 1;
    }

    long int n = atoi(argv[1]);
    float *vetor1, *vetor2;
    double produto_interno = 0;
    FILE *file;

    vetor1 = (float *)malloc(sizeof(float) * n);
    vetor2 = (float *)malloc(sizeof(float) * n);
    if (!vetor1 || !vetor2) {
        fprintf(stderr, "Memory allocation error\n");
        return 2;
    }

    srand(time(NULL));
    for (long int i = 0; i < n; i++) {
        vetor1[i] = (rand() % MAX) / 3.0;
        vetor2[i] = (rand() % MAX) / 3.0;
        produto_interno += vetor1[i] * vetor2[i];
    }

    file = fopen(argv[2], "wb");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        return 3;
    }

    fwrite(&n, sizeof(long int), 1, file);
    fwrite(vetor1, sizeof(float), n, file);
    fwrite(vetor2, sizeof(float), n, file);
    fwrite(&produto_interno, sizeof(double), 1, file);

    fclose(file);
    free(vetor1);
    free(vetor2);

    return 0;
}

