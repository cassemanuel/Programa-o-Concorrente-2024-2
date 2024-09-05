#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "timer.h"

typedef struct {
    int id;
    int linhas;
    int colunas1, colunas2;
    float *matriz1, *matriz2, *resultado;
    int num_threads;
} thread_args;

void* multiplicar_parte(void* args) {
    thread_args *dados = (thread_args*) args;
    int id = dados->id;
    int linhas = dados->linhas;
    int colunas1 = dados->colunas1;
    int colunas2 = dados->colunas2;
    float *matriz1 = dados->matriz1;
    float *matriz2 = dados->matriz2;
    float *resultado = dados->resultado;
    int num_threads = dados->num_threads;

    for (int i = id; i < linhas; i += num_threads) {
        for (int j = 0; j < colunas2; j++) {
            for (int k = 0; k < colunas1; k++) {
                resultado[i * colunas2 + j] += matriz1[i * colunas1 + k] * matriz2[k * colunas2 + j];
            }
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Uso: %s <arquivo1> <arquivo2> <arquivoSaida> <num_threads>\n", argv[0]);
        return 1;
    }

    int linhas1, colunas1, linhas2, colunas2, num_threads;
    float *matriz1, *matriz2, *resultado;
    FILE *arquivo1, *arquivo2, *arquivoSaida;
    pthread_t *threads;
    thread_args *args;
    double inicio, fim;

    num_threads = atoi(argv[4]);
    threads = (pthread_t*) malloc(num_threads * sizeof(pthread_t));
    args = (thread_args*) malloc(num_threads * sizeof(thread_args));

    arquivo1 = fopen(argv[1], "rb");
    arquivo2 = fopen(argv[2], "rb");
    if (!arquivo1 || !arquivo2) {
        fprintf(stderr, "Erro ao abrir os arquivos de entrada\n");
        return 1;
    }

    fread(&linhas1, sizeof(int), 1, arquivo1);
    fread(&colunas1, sizeof(int), 1, arquivo1);
    fread(&linhas2, sizeof(int), 1, arquivo2);
    fread(&colunas2, sizeof(int), 1, arquivo2);

    if (colunas1 != linhas2) {
        fprintf(stderr, "Dimensões incompatíveis para multiplicação\n");
        return 1;
    }

    matriz1 = (float*) malloc(linhas1 * colunas1 * sizeof(float));
    matriz2 = (float*) malloc(linhas2 * colunas2 * sizeof(float));
    resultado = (float*) calloc(linhas1 * colunas2, sizeof(float));

    fread(matriz1, sizeof(float), linhas1 * colunas1, arquivo1);
    fread(matriz2, sizeof(float), linhas2 * colunas2, arquivo2);

    fclose(arquivo1);
    fclose(arquivo2);

    GET_TIME(inicio);
    
    for (int i = 0; i < num_threads; i++) {
        args[i].id = i;
        args[i].linhas = linhas1;
        args[i].colunas1 = colunas1;
        args[i].colunas2 = colunas2;
        args[i].matriz1 = matriz1;
        args[i].matriz2 = matriz2;
        args[i].resultado = resultado;
        args[i].num_threads = num_threads;
        pthread_create(&threads[i], NULL, multiplicar_parte, (void*) &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    GET_TIME(fim);
    printf("Tempo de execução com %d threads: %lf segundos\n", num_threads, fim - inicio);

    arquivoSaida = fopen(argv[3], "wb");
    if (!arquivoSaida) {
        fprintf(stderr, "Erro ao abrir o arquivo de saída\n");
        return 1;
    }

    fwrite(&linhas1, sizeof(int), 1, arquivoSaida);
    fwrite(&colunas2, sizeof(int), 1, arquivoSaida);
    fwrite(resultado, sizeof(float), linhas1 * colunas2, arquivoSaida);

    fclose(arquivoSaida);

    free(matriz1);
    free(matriz2);
    free(resultado);
    free(threads);
    free(args);

    return 0;
}

