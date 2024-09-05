#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"  // Inclusão da biblioteca para medição de tempo

int main(int argc, char* argv[]) {
    int linhas1, colunas1, linhas2, colunas2;  // Dimensões das matrizes
    float *matriz1, *matriz2, *resultado;  // Ponteiros para as matrizes
    FILE *arquivo1, *arquivo2, *arquivoSaida;  // Arquivos para leitura e escrita
    size_t ret;  // Variável de controle de leitura/escrita
    double inicio, fim, delta;  // Variáveis para controle de tempo

    // Verifica se os argumentos foram passados corretamente
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <arquivo1> <arquivo2> <arquivoSaida>\n", argv[0]);
        return 1;
    }

    // Abertura dos arquivos binários
    arquivo1 = fopen(argv[1], "rb");
    if (!arquivo1) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", argv[1]);
        return 2;
    }
    arquivo2 = fopen(argv[2], "rb");
    if (!arquivo2) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", argv[2]);
        return 3;
    }
    
    // Leitura das dimensões da primeira matriz
    fread(&linhas1, sizeof(int), 1, arquivo1);
    fread(&colunas1, sizeof(int), 1, arquivo1);

    // Leitura das dimensões da segunda matriz
    fread(&linhas2, sizeof(int), 1, arquivo2);
    fread(&colunas2, sizeof(int), 1, arquivo2);

    // Verifica se as matrizes podem ser multiplicadas
    if (colunas1 != linhas2) {
        fprintf(stderr, "As dimensões das matrizes são incompatíveis para multiplicação\n");
        return 4;
    }

    // Alocação de memória para as matrizes e o resultado
    matriz1 = (float*) malloc(linhas1 * colunas1 * sizeof(float));
    matriz2 = (float*) malloc(linhas2 * colunas2 * sizeof(float));
    resultado = (float*) malloc(linhas1 * colunas2 * sizeof(float));

    if (!matriz1 || !matriz2 || !resultado) {
        fprintf(stderr, "Erro na alocação de memória\n");
        return 5;
    }

    // Leitura dos valores das matrizes
    fread(matriz1, sizeof(float), linhas1 * colunas1, arquivo1);
    fread(matriz2, sizeof(float), linhas2 * colunas2, arquivo2);

    // Fecha os arquivos de entrada
    fclose(arquivo1);
    fclose(arquivo2);

    // Inicializa a matriz de resultado com zeros
    for (int i = 0; i < linhas1 * colunas2; i++) {
        resultado[i] = 0.0;
    }

    // Início da medição de tempo para a multiplicação
    GET_TIME(inicio);

    // Multiplicação de matrizes
    for (int i = 0; i < linhas1; i++) {
        for (int j = 0; j < colunas2; j++) {
            for (int k = 0; k < colunas1; k++) {
                resultado[i * colunas2 + j] += matriz1[i * colunas1 + k] * matriz2[k * colunas2 + j];
            }
        }
    }

    // Fim da medição de tempo
    GET_TIME(fim);
    delta = fim - inicio;
    printf("Tempo de execução da multiplicação: %lf segundos\n", delta);

    // Abertura do arquivo de saída para escrever o resultado
    arquivoSaida = fopen(argv[3], "wb");
    if (!arquivoSaida) {
        fprintf(stderr, "Erro ao abrir o arquivo %s\n", argv[3]);
        return 6;
    }

    // Escrita das dimensões da matriz resultado
    fwrite(&linhas1, sizeof(int), 1, arquivoSaida);
    fwrite(&colunas2, sizeof(int), 1, arquivoSaida);

    // Escrita dos valores da matriz resultado no arquivo
    fwrite(resultado, sizeof(float), linhas1 * colunas2, arquivoSaida);

    // Fecha o arquivo de saída
    fclose(arquivoSaida);

    // Libera a memória alocada
    free(matriz1);
    free(matriz2);
    free(resultado);

    return 0;
}

