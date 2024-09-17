#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

long int soma = 0; // Variável compartilhada entre as threads
pthread_mutex_t mutex; // Mutex para exclusão mútua
pthread_cond_t cond_print;   // Condição para sinalizar impressão
pthread_cond_t cond_continue; // Condição para sinalizar continuação
int contador_multiplos = 0; // Contador de múltiplos de 10 impressos
int multiples_processed = 0; // Flag para indicar se um múltiplo está sendo processado

// Função executada pelas threads de soma
void *ExecutaTarefa(void *arg) {
    long int id = (long int) arg;
    printf("Thread : %ld está executando...\n", id);

    while (1) {
        pthread_mutex_lock(&mutex);
        
        // Verifica se já processou 20 múltiplos
        if (contador_multiplos >= 20) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        soma++; // Incrementa a variável compartilhada

        // Se a soma for múltiplo de 10 e não está sendo processada
        if (soma % 10 == 0 && multiples_processed == 0) {
            multiples_processed = 1; // Marca que um múltiplo está sendo processado
            pthread_cond_signal(&cond_print); // Sinaliza a thread extra para imprimir
            pthread_cond_wait(&cond_continue, &mutex); // Espera a thread extra imprimir
        }

        pthread_mutex_unlock(&mutex);
    }

    printf("Thread : %ld terminou!\n", id);
    pthread_exit(NULL);
}

// Função executada pela thread extra
void *extra(void *args) {
    printf("Extra : está executando...\n");
    while (1) {
        pthread_mutex_lock(&mutex);

        // Espera até que um múltiplo de 10 seja sinalizado
        while (multiples_processed == 0 && contador_multiplos < 20) {
            pthread_cond_wait(&cond_print, &mutex);
        }

        // Verifica se já processou 20 múltiplos
        if (contador_multiplos >= 20) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        // Verifica se o valor é múltiplo de 10
        if (soma % 10 == 0) {
            // Imprime a soma
            printf("soma = %ld \n", soma);
            contador_multiplos++;
        }

        multiples_processed = 0; // Reseta a flag para permitir a continuação

        pthread_cond_signal(&cond_continue); // Sinaliza as threads principais para continuarem
        pthread_mutex_unlock(&mutex);
    }

    printf("Extra : terminou!\n");
    pthread_exit(NULL);
}


// Função principal
int main(int argc, char *argv[]) {
    pthread_t *tid; // Identificadores das threads no sistema
    int nthreads; // Número de threads (passado via linha de comando)

    // Verifica os parâmetros de entrada
    if (argc < 2) {
        printf("Uso: %s <numero de threads>\n", argv[0]);
        return 1;
    }
    nthreads = atoi(argv[1]);

    // Aloca as estruturas de threads
    tid = (pthread_t*) malloc(sizeof(pthread_t) * (nthreads + 1));
    if (tid == NULL) { 
        perror("Erro no malloc"); 
        return 2; 
    }

    // Inicializa o mutex e as variáveis de condição
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_print, NULL);
    pthread_cond_init(&cond_continue, NULL);

    // Cria as threads principais
    for (long int t = 0; t < nthreads; t++) {
        if (pthread_create(&tid[t], NULL, ExecutaTarefa, (void *)t)) {
            perror("Erro na criação da thread ExecutaTarefa");
            exit(EXIT_FAILURE);
        }
    }

    // Cria a thread extra
    if (pthread_create(&tid[nthreads], NULL, extra, NULL)) {
        perror("Erro na criação da thread extra");
        exit(EXIT_FAILURE);
    }

    // Espera todas as threads terminarem
    for (int t = 0; t < nthreads + 1; t++) {
        if (pthread_join(tid[t], NULL)) {
            perror("Erro no pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    // Finaliza o mutex e as variáveis de condição
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_print);
    pthread_cond_destroy(&cond_continue);

    printf("Valor final de 'soma' = %ld\n", soma);
    free(tid); // Libera a memória alocada para as threads
    return 0;
}

