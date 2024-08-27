#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>

typedef struct {
    float *vetor1;
    float *vetor2;
    long int inicio;
    long int fim;
    double resultado_parcial;
} ThreadData;

void *calcular_produto_interno(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    double soma = 0;
    for (long int i = data->inicio; i < data->fim; i++) {
        soma += data->vetor1[i] * data->vetor2[i];
    }
    data->resultado_parcial = soma;
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input file> <threads>\n", argv[0]);
        return 1;
    }

    long int n, nthreads;
    float *vetor1, *vetor2;
    double produto_interno_sequencial, produto_interno_concorrente = 0;
    FILE *file;

    nthreads = atoi(argv[2]);

    file = fopen(argv[1], "rb");
    if (!file) {
        fprintf(stderr, "Error opening file\n");
        return 2;
    }

    fread(&n, sizeof(long int), 1, file);
    vetor1 = (float *)malloc(sizeof(float) * n);
    vetor2 = (float *)malloc(sizeof(float) * n);
    fread(vetor1, sizeof(float), n, file);
    fread(vetor2, sizeof(float), n, file);
    fread(&produto_interno_sequencial, sizeof(double), 1, file);
    fclose(file);

    pthread_t threads[nthreads];
    ThreadData thread_data[nthreads];
    long int chunk = n / nthreads;

    for (long int i = 0; i < nthreads; i++) {
        thread_data[i].vetor1 = vetor1;
        thread_data[i].vetor2 = vetor2;
        thread_data[i].inicio = i * chunk;
        thread_data[i].fim = (i == nthreads - 1) ? n : (i + 1) * chunk;
        pthread_create(&threads[i], NULL, calcular_produto_interno, &thread_data[i]);
    }

    for (long int i = 0; i < nthreads; i++) {
        pthread_join(threads[i], NULL);
        produto_interno_concorrente += thread_data[i].resultado_parcial;
    }

    printf("Produto Interno Sequencial: %.6f\n", produto_interno_sequencial);
    printf("Produto Interno Concorrente: %.6f\n", produto_interno_concorrente);
    printf("Variação Relativa: %.6f\n",
           fabs((produto_interno_sequencial - produto_interno_concorrente) / produto_interno_sequencial));

    free(vetor1);
    free(vetor2);

    return 0;
}

