#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

#define NUM_THREADS 2
#define BUFFER_SIZE 100

char input_buffer[BUFFER_SIZE];
char output_buffer[BUFFER_SIZE * 2]; 
sem_t sem_input_empty;
sem_t sem_input_full;
sem_t sem_output_empty;
sem_t sem_output_full;
char* input_filename = "entrada.txt"; // define o nome do arquivo diretamente
int is_file_done = 0;
int is_output_end = 0;
pthread_mutex_t mutex_lock;

// funcao que le o arquivo e coloca no input_buffer
void* file_reader(void* arg){
	size_t current_bytes = 0;
	FILE* file = fopen(input_filename, "r");
	while(1){
		sem_wait(&sem_input_empty); // espera o input_buffer estar vazio
		current_bytes = fread(&input_buffer, sizeof(char), BUFFER_SIZE - 1, file); // le do arquivo e armazena no buffer
		if(current_bytes < BUFFER_SIZE - 1){ // verifica se terminou o arquivo
			input_buffer[current_bytes + 1] = '\0'; // marca o final do buffer
			pthread_mutex_lock(&mutex_lock);
			is_file_done = 1; // indica que a leitura do arquivo acabou
			pthread_mutex_unlock(&mutex_lock);
			sem_post(&sem_input_full); // libera o semaforo para a proxima thread
			break;	
		}
		sem_post(&sem_input_full); // libera o semaforo para a proxima thread
	}
	fclose(file);	
    pthread_exit(NULL);
}

// funcao que processa os dados do input_buffer e armazena no output_buffer
void* data_processor(void* arg){	
	int line_count = 0;
	int offset = 0;
	int char_count = 0;
	int line_break_counter = 0; 
	while(1){
		sem_wait(&sem_input_full); // espera o input_buffer estar cheio
		sem_wait(&sem_output_empty); // espera o output_buffer estar vazio
		offset = 0;
		for(int i = 0; i < BUFFER_SIZE - 1; i++){
			if(input_buffer[i] == '\0'){ // se encontrar o final do buffer, termina
				output_buffer[i + offset] = '\0';
				break;
			}
			// adiciona uma quebra de linha apos (2*line_break_counter + 1) caracteres
			if(line_count == 2 * line_break_counter + 1 && line_break_counter <= 10){
				output_buffer[i + offset] = '\n';
				offset++;
				line_count = 0; // reinicia a contagem
                char_count++;
				line_break_counter++;
			} else if(line_break_counter > 10 && line_count % 10 == 0){
				// apos o limite, insere uma quebra de linha a cada 10 caracteres
				output_buffer[i + offset] = '\n';
				offset++;
				line_count = 0;
                char_count++;
			}
			output_buffer[i + offset] = input_buffer[i];
            char_count++;
			line_count++;
		}
		output_buffer[BUFFER_SIZE + offset] = '\0'; // finaliza o buffer de saida
		sem_post(&sem_input_empty); // libera o semaforo para a thread de leitura
		sem_post(&sem_output_full); // libera o semaforo para a thread de impressao
		pthread_mutex_lock(&mutex_lock);
		if(is_file_done){ // verifica se a leitura ja foi finalizada
			pthread_mutex_unlock(&mutex_lock);
			break;
		}
		pthread_mutex_unlock(&mutex_lock);
	}
	pthread_mutex_lock(&mutex_lock);
	is_output_end = 1; // indica que o processamento acabou
	pthread_mutex_unlock(&mutex_lock);
	pthread_exit(NULL);
}

// funcao que imprime o output_buffer na tela
void* output_printer(void* arg){
	int printed_chars = 0;
	while(1){
		sem_wait(&sem_output_full); // espera o output_buffer estar cheio
		printed_chars += printf("%s", output_buffer); // imprime o buffer de saida
		sem_post(&sem_output_empty); // libera o semaforo para a thread de processamento
		pthread_mutex_lock(&mutex_lock);
		if(is_output_end){ // verifica se o processamento acabou
			pthread_mutex_unlock(&mutex_lock);
			break;
		}
		pthread_mutex_unlock(&mutex_lock);
	}
	pthread_exit(NULL);
}

int main(){
 	pthread_t threads[NUM_THREADS + 1];  // cria um array de threads
	pthread_mutex_init(&mutex_lock, NULL); // inicializa o mutex
	sem_init(&sem_input_empty, 0, 1); // inicializa o semaforo do input_buffer vazio
	sem_init(&sem_output_empty, 0, 1); // inicializa o semaforo do output_buffer vazio
	sem_init(&sem_input_full, 0, 0); // inicializa o semaforo do input_buffer cheio
	sem_init(&sem_output_full, 0, 0); // inicializa o semaforo do output_buffer cheio

	pthread_create(&threads[0], NULL, file_reader, NULL); // cria a thread de leitura
	pthread_create(&threads[1], NULL, data_processor, NULL); // cria a thread de processamento
	pthread_create(&threads[2], NULL, output_printer, NULL); // cria a thread de impressao

	for(int i = 0; i < NUM_THREADS + 1; i++){
		if(pthread_join(threads[i], NULL)){ // espera as threads terminarem
			fprintf(stderr, "erro ao finalizar thread %d\n", i);
			exit(50);
		}
	}

	// destroi os semaforos e o mutex
	sem_destroy(&sem_input_full);
	sem_destroy(&sem_input_empty);
	sem_destroy(&sem_output_full);
	sem_destroy(&sem_output_empty);
	pthread_mutex_destroy(&mutex_lock);

	return 0;
}

