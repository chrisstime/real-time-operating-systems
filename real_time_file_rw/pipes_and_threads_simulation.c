//48450 Real-Time Operating Systems Assignment
//Date: 1st May 2018

// to compile gcc 11986282_Prg1.c -o prg_1.c -pthread -lrt
// to run the compiled file ./prg_1.c

//Included libraries
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>

#define DATA_FILE "data.txt"
#define SRC_FILE "src.txt"
#define BUFFER 100

/* Timer variables */
clock_t start, stop;
double program_run_time;

/* Semaphore and Thread variables */
sem_t write_semaphore, read_semaphore, justify_semaphore;
pthread_t thread_a, thread_b, thread_c;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* Pipe declaration */
int fd[2];

/* Function Declarations */
void *ThreadA(void *A);
void *ThreadB(void *B);
void *ThreadC(void *C);

// file struct for src and data files
typedef struct{
	FILE* data;
	FILE* src;
	char line_text[BUFFER];
	int end_of_file_flag;
}file_struct;

// create an instance of the file struct
file_struct textFile;

/* define shared memory object size*/
#define SIZE 4096
/* define shared memeory name*/
#define shared_name "shared_memory"

/* shared memory file descriptor */
int shm_mem;
/* memory pointer */
void *mem_ptr;

int ProgramStartMenu(){
	int exitProgram = 0;
	char user_input[BUFFER];

	printf("Program will read data.txt file. Make sure it is located in the same directory.\n");
	while(1){
		printf("Would you like to proceed? [Y/N] ");
		scanf("%s[^\n]", user_input);

		/* check if user input is Y or N*/
		if (strstr(user_input, "Y") != 0) break;
		/* check if user input is N then exit program */
		else if (strstr(user_input, "N") != 0){
			printf("Terminating program...\n");
			exitProgram = 1;
			break;
		}
		else printf("Please type a valid input. Y/N only.\n");
	}
	return exitProgram;
}

/* Calculates program run time and prints for user to view. */
void StopClock(){
	stop = clock();
	program_run_time = ((double)(stop - start)) / CLOCKS_PER_SEC;
	printf("Program Run Time: %f seconds.\n", program_run_time);
}

/* Function creates each thread.
*  Returns 0 as soon as it fails to create a thread (to save resources).
*/
int InitThread(){
	if (pthread_create(&thread_a, NULL, &ThreadA, NULL) != 0) return 1;
	if (pthread_create(&thread_b, NULL, &ThreadB, NULL) != 0) return 1;
	if (pthread_create(&thread_c, NULL, &ThreadC, NULL) != 0) return 1;
	return 0;
}

/* Function creates each semaphore.
*  Returns 0 as soon as it fails to create a semaphore (to save resources).
*/
int InitSemaphore(){
	/* int sem_init(sem_t *sem, int pshared, unsigned int value)
	* If Pshared value 0; semaphore is shared between the threads of a process
  * If Pshared value nonzero; semaphore is shared between processes (shared memory location)
	*/
	if (sem_init(&write_semaphore, 0, 1) != 0) return 1;
	if (sem_init(&read_semaphore, 0, 0) != 0) return 1;
	if (sem_init(&justify_semaphore, 0, 0) != 0) return 1;
	return 0;
}

/* Function joins threads.
*  Returns 0 as soon as joining fails (to save resources).
*/
int JoinThreads(){
	if (pthread_join(thread_a, NULL) != 0) return 1;
	if (pthread_join(thread_b, NULL) != 0) return 1;
	if (pthread_join(thread_c, NULL) != 0) return 1;
	return 0;
}

int DestroySemaphores(){
	return sem_destroy(&read_semaphore) && sem_destroy(&write_semaphore) &&
		sem_destroy(&justify_semaphore);
}

/* Closes pipes, returns 0 on success.
*/
int ClosePipe(){
	return close(fd[0]) && close(fd[1]);
}

/* Exit function for threads. Params are the thread name and semaphore to post.
*/
void exitThread(char threadName, sem_t *semaphore){
	/*  */
	if (semaphore != NULL) sem_post(semaphore);
	printf("Exiting Thread %c...\n\n", threadName);
	pthread_exit(NULL);
}

/* Function to create and write to shared memory. The shared memory location is hard coded.
*/
int writeToSharedMemory(){
	/* create shared memory object and set object size*/
	shm_mem = shm_open(shared_name, O_CREAT | O_RDWR, 0666);
	if(shm_mem == -1) return 1;

	ftruncate(shm_mem, SIZE);

	/* memory map */
	mem_ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_mem, 0);

	/* write program run time to the memory object */
	sprintf(mem_ptr, "%f", program_run_time);
	printf("Write program run time to shared memory success!\n\n");
	return 0;
}

/* Main program
*/
int main(void){
	start = clock(); /* Starts the clock. */
	pipe(fd);
	textFile.end_of_file_flag = 1; /* Set end of file flag to 1 (false) */

	if (ProgramStartMenu() != 1){

		/* Initialise semaphores & threads. Returns error message if initialisation fails. */
		if (InitSemaphore() != 0) perror("Error initialising semaphores.\n");
		if (InitThread() != 0) perror("Error initialising threads.\n");
		if (JoinThreads() != 0) perror("Thread execution error.\n");

		/* After program clean-up: close all pipes and semaphores. */
		if (ClosePipe() != 0) perror("Error closing pipes.");
		if (DestroySemaphores() != 0) perror("Error destroying semaphores.");

		/* Stop the clock and print cpu run time. */
		StopClock();
		/* call shared memory function to store program run time */
		if (writeToSharedMemory() != 0) perror("Write to shared memory failed.");
		printf("Program finished. The src.txt is ready. Terminating...\n");
	}
	exit(0);
}

/* Read Thread */
void* ThreadA(void *A){
	char dataString[BUFFER];
	/* open data.txt file and store to textFile->data */
	textFile.data = fopen(DATA_FILE, "r");

	if (textFile.data){
		/* while text line is still present */
		while(fgets(dataString, sizeof(dataString), textFile.data)){
			/* hold write semaphore */
			sem_wait(&write_semaphore);
			/* print what text detected by Pipe for user reference and pass to pipe */
			printf("[Thread A] Writing to Pipe: %s\n", dataString);
			write(fd[1], dataString, sizeof(dataString));
			/* post read semaphore to signal thread A is complete */
			sem_post(&read_semaphore);

		} fclose(textFile.data);
	}
	/* signal end of file */
	textFile.end_of_file_flag = 0;
	/* signal read semaphore as finished and exit thread */
	exitThread('A', &read_semaphore);
}

/* Justify Header Thread */
void* ThreadB(void *B){
	while(1){
		char dataString[BUFFER];
		/* awaiting read semaphore post */
		sem_wait(&read_semaphore);
		/* lock thread */
		pthread_mutex_lock(&mutex);

		/* read dataString from pipe */
		read(fd[0], dataString, BUFFER);
		/* unlock thread */
		pthread_mutex_unlock(&mutex);
		/* print what is detected by Pipe for user reference */
		printf("[Thread B] Reading Pipe Header: %s\n", dataString);
		/* copy string to textFile->line_text for writing to src.txt file */
		strcpy(textFile.line_text, dataString);
		/* signal that thread B is finished */
		sem_post(&justify_semaphore);
		/* if end of file flag is raised, break out of the while loop and exit thread */
		if (textFile.end_of_file_flag == 0) break;
	}
	/* signal justify sempahore and exit thread */
	exitThread('B', &justify_semaphore);
}

/* Write Thread */
void* ThreadC(void *C){
	/* open src.txt file for writing */
	textFile.src = fopen(SRC_FILE, "w");
	/* indicator to start writing to file */
	int write_flag = 0;

	while(1){
		/* awaiting justify post */
		sem_wait(&justify_semaphore);
		/* lock thread. Important if we're going to write to file. */
		pthread_mutex_lock(&mutex);

		/* if write flag is set, start writing to src.txt file */
		if(write_flag){
			printf("[Thread C] Writing to source file: %s\n", textFile.line_text);
			fprintf(textFile.src, "%s\n", textFile.line_text);
		}

	  /* unlock thread */
		pthread_mutex_unlock(&mutex);

		/* check if content starts with end_header. If it does set write flag */
		if(strstr(textFile.line_text, "end_header") != 0){
			printf("[Thread C] End header detected: %s\n", textFile.line_text);
			write_flag = 1;
		}
		/* if end of file flag is set, break out of the while loop and exit thread C */
		if(textFile.end_of_file_flag == 0) break;
		/* if end of file is not reached, signal write semaphore process is finished */
		sem_post(&write_semaphore);

	}
	/* close the source file and exit the thread */
	fclose(textFile.src);
	exitThread('C', NULL);
}
