#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

pthread_rwlock_t rwlock;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t pode_ler = PTHREAD_COND_INITIALIZER;
pthread_cond_t pode_escrever = PTHREAD_COND_INITIALIZER;

int leitores_ativos = 0;
int escritores_esperando = 0;
int escritores_ativos = 0;

// controla quando a leitura pode começar
void iniciar_leitura(int id) {
    pthread_mutex_lock(&mutex);
    
    // se tem escritores esperando ou escrevendo, a thread leitora espera
    while (escritores_esperando > 0 || escritores_ativos > 0) {
        printf("Leitor %d esperando, escritores estão na frente!\n", id); // mostra quem tem prioridade
        pthread_cond_wait(&pode_ler, &mutex);
    }
    
    // leitura começa
    leitores_ativos++;
    printf("Leitor %d começou a ler\n", id); // quando é o início de leitura
    
    pthread_mutex_unlock(&mutex);
}

// termina a leitura
void terminar_leitura(int id) {
    pthread_mutex_lock(&mutex);
    
    // Leitura terminou, decrementa o contador de leitores
    leitores_ativos--;
    
    // se não tem mais leitores sinaliza que escritores podem começar
    if (leitores_ativos == 0) {
        pthread_cond_signal(&pode_escrever);
    }
    
    printf("Leitor %d terminou de ler\n", id); // fim de leitura
    
    pthread_mutex_unlock(&mutex);
}

// controla quando a escrita pode começar
void iniciar_escrita(int id) {
    pthread_mutex_lock(&mutex);
    
    // Escritor se anuncia e entra na fila de espera
    escritores_esperando++;
    
    // se tem alguém escrevendo ou lendo, espera
    while (escritores_ativos > 0 || leitores_ativos > 0) {
        printf("Escritor %d esperando, leitores ou outro escritor em andamento\n", id); // Log para mostrar espera
        pthread_cond_wait(&pode_escrever, &mutex);
    }
    
    // Escritor pode começar
    escritores_esperando--;
    escritores_ativos++;
    
    printf("Escritor %d começou a escrever\n", id); // início de escrita
    
    pthread_mutex_unlock(&mutex);
}

// termina a escrita
void terminar_escrita(int id) {
    pthread_mutex_lock(&mutex);
    
    // Escritor terminou de escrever
    escritores_ativos--;
    
    // se tem escritores esperando sinaliza para outro escritor
    if (escritores_esperando > 0) {
        pthread_cond_signal(&pode_escrever);
    } else {
        // Se não tem escritores esperando libera os leitores
        pthread_cond_broadcast(&pode_ler);
    }
    
    printf("Escritor %d terminou de escrever\n", id); // fim de escrita
    
    pthread_mutex_unlock(&mutex);
}

// Função executada peLas threads leitoras
void *leitor(void *arg) {
    int id = *(int *)arg;
    
    iniciar_leitura(id); // simulação da leitura
    terminar_leitura(id);
    
    return NULL;
}

// Função executada por threads escritoras
void *escritor(void *arg) {
    int id = *(int *)arg;
    
    iniciar_escrita(id); // Simulação da escrita
    terminar_escrita(id);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t leitores[5], escritores[3];
    int id_threads[5] = {1, 2, 3, 4, 5};

    // criação das threads escritoras
    for (int i = 0; i < 3; i++) {
        pthread_create(&escritores[i], NULL, escritor, &id_threads[i]);
    }

    // criação das threads leitoras
    for (int i = 0; i < 5; i++) {
        pthread_create(&leitores[i], NULL, leitor, &id_threads[i]);
    }

    // aguarda as threads escritoras terminarem
    for (int i = 0; i < 3; i++) {
        pthread_join(escritores[i], NULL);
    }

    // aguarda as threads leitoras terminarem
    for (int i = 0; i < 5; i++) {
        pthread_join(leitores[i], NULL);
    }

    return 0;
}
