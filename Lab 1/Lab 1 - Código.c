#include <stdio.h>       // biblio principal (printf, scanf, etc)
#include <stdlib.h>      // malloc
#include <pthread.h>

// passar argumentos para as threads
typedef struct {
    int *vetor;          // ponteiro para o vetor que as threads vao modificar
    int inicio;          // posicao inicial que a thread vai trabalhar
    int fim;             // posicao final que a thread vai trabalhar
} ThreadArgs;

// parte que foi pedida (somar em cada thread)
void *soma1(void *args) {
    ThreadArgs *targs = (ThreadArgs *)args; // cast do argumento para a estrutura ThreadArgs
    for (int i = targs->inicio; i < targs->fim; i++) { // somar 1 em cada elemento da parte do vetor dessa thread
        targs->vetor[i] += 1;
    }
    pthread_exit(NULL);  // termina a thread
}

// inicializa o vetor só com numeros de 0 a N-1
void inicializa_vetor(int *vetor, int N) {
    for (int i = 0; i < N; i++) {
        vetor[i] = i;
    }
}

// verificacao de segurança (se todas as posições que contém um valor foram incrementadas)
void verifica_vetor(int *vetor, int N) {
    for (int i = 0; i < N; i++) {
        if (vetor[i] != i + 1) { // posicao i é (i + 1)
            printf("Erro na posição %d: esperado %d, encontrado %d\n", i, i + 1, vetor[i]); // imprime se tiver erro
            return;
        }
    }
    printf("Vetor atualizado corretamente!\n"); // vasco da gama
}

int main(int argc, char *argv[]) {
    if (argc != 3) { // pediu 2 argumentos a serem passados, por isso nao usar Scanf
        printf("Uso: %s <N> <M>\n", argv[0]); // mostra como usar o programa
        return 1;
    }

    int N = atoi(argv[1]); // ascII to integer - converte o primeiro argumento (N)
    int M = atoi(argv[2]); // aqui é o (N)

    int *vetor = (int *)malloc(N * sizeof(int)); // aloca memoria para o vetor de N inteiros
    inicializa_vetor(vetor, N);

    pthread_t threads[M]; // cria um array para guardar os identificadores das threads. "PID das threads"
    ThreadArgs targs[M]; // usa aquele struct para várias threads receberem parâmetro de entrada

    int elementos_por_thread = N / M;
    int resto = N % M; // resto de N/M, ultima thread faz oq sobrar
    int inicio = 0;

    // cria as threads
    for (int i = 0; i < M; i++) {
        targs[i].vetor = vetor; // passa o ponteiro do vetor para o struct
        targs[i].inicio = inicio; // define onde essa thread começa (0)
        targs[i].fim = inicio + elementos_por_thread; // define onde essa thread termina (0 + (N/M))
        if (resto > 0) { // pega o que sobrou - resto
            targs[i].fim += 1;
            resto--; // decrementa o resto, pro proximo nao pegar mais
        }
        inicio = targs[i].fim; // atualiza o inicio para a proxima thread
        pthread_create(&threads[i], NULL, soma1, (void *)&targs[i]); // cria a thread passando a funcao soma1 e os argumentos
    }

    // espera todas as threads terminarem de executar suas tarefas
    for (int i = 0; i < M; i++) {
        pthread_join(threads[i], NULL); // pausa a main até a thread[i] terminar
    }

    verifica_vetor(vetor, N);

    free(vetor); // libera a memoria alocada para o vetor
    return 0;
}
