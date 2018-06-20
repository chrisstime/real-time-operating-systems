//48450 Real-Time Operating Systems
//Date: 22nd May 2018

// to compile gcc 11986282_A3_Prg1.c -o prg_1.c -pthread -lrt
// to run the compiled file sudo ./prg_1.c

/* Include required libraries for the program to run */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

/* Static variable declarations */
#define PID_COUNT 7
#define BUFFER_SIZE 256
#define OUT_FILE "output.txt"
#define FIFO_PATH "/myfifo"

/* Function Prototype Declarations */
void* SrtfThread();
void* WriteThread();

/* Semaphore, Thread, file and mutex lock variables */
sem_t srtf_semaphore, write_semaphore;
pthread_t srtf_thread, write_thread;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
FILE* file;

/* Process struct. Contains variables required per process. */
typedef struct{
  int arrival_time;
  int initial_burst_time;
  int time_remaining;
  int wait_time;
  int completed;
  int turnaround_time;
} p_struct;

/* create 7 instances of the process struct for each process */
p_struct process[PID_COUNT];

/* Data from A3 spec. Arrival Time & Burst Time for each process */
int input_data[7][2] = {
                        { 8, 10 },
                        { 10, 3 },
                        { 14, 7 },
                        { 9, 5 },
                        { 16, 4 },
                        { 21, 6 },
                        { 26, 2 }
                       };

/* Method to handle initial user input to run the program.
*/
int StartMenu(){
	int exitProgram = 0;
	char user_input[BUFFER_SIZE];

	printf("Program starting...\n");
	while(1){
		printf("Would you like to proceed? [y/n] ");
		scanf("%s[^\n]", user_input);

		/* check if user input is y or n*/
		if (strstr(user_input, "y") != 0) break;
		/* check if user input is N then exit program */
		else if (strstr(user_input, "n") != 0){
			printf("Terminating program...\n");
			exitProgram = 1;
			break;
		}
		else printf("Please type a valid input. y/n only.\n");
	}
	return exitProgram;
}

/* Function creates each thread.
*  Returns 0 as soon as it fails to create a thread (to save resources).
*/
int InitThread(){
	if (pthread_create(&srtf_thread, NULL, &SrtfThread, NULL) != 0) return 1;
	if (pthread_create(&write_thread, NULL, &WriteThread, NULL) != 0) return 1;
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
	if (sem_init(&srtf_semaphore, 0, 1) != 0) return 1;
	if (sem_init(&write_semaphore, 0, 0) != 0) return 1;
	return 0;
}

/* Function joins threads.
*  Returns 1 as soon as joining fails (to save resources).
*/
int JoinThreads(){
	if (pthread_join(srtf_thread, NULL) != 0) return 1;
	if (pthread_join(write_thread, NULL) != 0) return 1;
	return 0;
}

/* Function destorys semaphores on program termination.
*  Returns 0 on success.
*/
int DestroySemaphores(){
	return sem_destroy(&srtf_semaphore) && sem_destroy(&write_semaphore);
}

/* Function to assign burst and arrival times to each process as per Assignment 3 spec
*/
void CreateProcesses(){
  int n;
  int count;
  /* loop through each process struct and assign the correct values*/
  for (n = 0; n < PID_COUNT ; n++){
    process[n].arrival_time = input_data[n][0];
    process[n].initial_burst_time = input_data[n][1];
    process[n].time_remaining = input_data[n][1];
    process[n].completed = 0;
    process[n].turnaround_time = 0;
  }
}

/* Function to unlink created FIFO if the program has been run before.
*  Returns 0 if unlinking is successful or if fifo is ok to access to the program specified path
*/
int FifoCleanUp(){
  if (!access(FIFO_PATH, F_OK))
    return unlink(FIFO_PATH);
  return 0;
}

int main(void){
  if (StartMenu() != 1){
    /* Create the processes */
    CreateProcesses();

		/* Precaution to unlink Fifo created if it failed to unlink on last program run. */
    if (FifoCleanUp() != 0) perror("Error cleaning up FIFO. ");
    /* Initialise semaphores, threads and FIFO. Returns error message if initialisation fails. */
    if (mkfifo(FIFO_PATH, PROT_READ | PROT_WRITE) == -1) perror("Error initialising FIFO. ");
		if (InitSemaphore() != 0) perror("Error initialising semaphores. ");
		if (InitThread() != 0) perror("Error initialising threads. ");
		if (JoinThreads() != 0) perror("Thread execution error. ");

		/* After program clean-up: close all pipes and semaphores. */
		if (DestroySemaphores() != 0) perror("Error destroying semaphores. ");
		printf("Program finished. Terminating...\n");
	}
  exit(0);
}

/* Function to sort through the process and order them by arrival_time.
*  Bubble sorting algorithm was chosen for this as the sorting size is relatively small
*  (only 7 processes to sort) therefore this method would be the most efficient
*  compared to qsort and other algorithms.
*/
void BubbleSortProcesses(){
   int count;
   int index;
   /* initialise temporary process struct for as placeholder for swapping */
   p_struct temp;
   /* loop through the process and compare arrival_time of current process and process after */
   for (index = 0; index < PID_COUNT; index++)
     for (count = index+1; count < PID_COUNT; count++)
       if (process[count].arrival_time < process[index].arrival_time){
         /* swap */
         temp = process[index];
         process[index] = process[count];
         process[count] = temp;
       }
}

/* Function to calculate total process run time by adding all the Burst
* times from all the processes, starting from first process arrival.
* Returns process run time.
*/
int ProcessRunTime(){
  int count;
  int run_time = process[0].arrival_time;

  for (count = 0 ; count < PID_COUNT ; count ++){
    run_time += process[count].initial_burst_time;
  }
  return run_time;
}

/* Calculate the average turnaround time for all processes
*  Returns average turnaround time for all processes. This must be run first
*  before AverageWaitTime() as that function uses turnaround_time
*/
double AverageTurnaroundTime(){
  double total_turnaround_time = 0;
  double avg_turnaround_time;
  int n;

  for (n = 0; n < PID_COUNT ; n++){
    /* turn around time per process is Completed time - Arrival Time */
    process[n].turnaround_time = process[n].completed - process[n].arrival_time;
    /* Add process turnaround time to total turnaround time */
    total_turnaround_time += process[n].turnaround_time;
  }
  /* return the average turnaround time for the 7 processes */
  return (double)total_turnaround_time / PID_COUNT;
}

/* Function to calculate average wait time of all processes
*  Returns average wait time.
*/
double AverageWaitTime(){
  double total_wait_time = 0;
  int n;

  for (n = 0; n < PID_COUNT ; n++){
    /* wait time per process is Turnaround time - (initial) Burst time */
    process[n].wait_time = process[n].turnaround_time - process[n].initial_burst_time;
    /* add process wait time to total wait time */
    total_wait_time += process[n].wait_time;
  }
  /* return the average wait time for the 7 processes */
  return (double)total_wait_time / PID_COUNT;
}

/* Function to calculate the current process with the lowest remaining burst time.
*  Returns the index for that process.
*/
int LowestRemainingBurst(int running_processes){
  int index = 0;
  int n;

  for (n = 0; n < running_processes ; n++){
    /* if processes isn't marked as completed */
    if (process[n].completed == 0)
      /* and time remaining on the processes is less than or equal to current process running */
      if (process[n].time_remaining <= process[index].time_remaining)
        /* make that process the new index to run */
        index = n;
  }
  return index;
}

/* Function to print process attributes and finish times to terminal.
*/
void ResultsToTerminal(){
  int i;
  printf("RESULTS\n");
  for (i = 0; i < PID_COUNT; i++){
    printf("P%d:\n Arrival Time: %d\n Burst Time: %d ms\n Finish Time: %d ms\n",
            i+1, process[i].arrival_time,
            process[i].initial_burst_time, process[i].completed);
  }
}

/* This is the Scheduling Thread to calculate the shortest remaining time first.
*/
void* SrtfThread(){
  /* initialise all required variables */
  int processes_remaining = PID_COUNT;
  /* get the total run time for all the processes */
  int total_time = ProcessRunTime();
  int running_processes = 0;
  int counter = 0;
  int current_p = 0;
  int i;

  /* wait for the call to run this thread. Called in main() */
  sem_wait(&srtf_semaphore);

  /* bubble sort through the processes first to order them by arrival time */
  BubbleSortProcesses();

  while(counter <= total_time){
    /* if the process is specified to arrive at this time */
    if(process[running_processes].arrival_time == counter){
      /* and while the total running processes is less than process count of 7 */
      if(running_processes < PID_COUNT)
        /* increment processes count to indicate another processes has started at this time */
        running_processes++;
      /* go through the total processes running, make current_p the processes with lowest burst time */
      current_p = LowestRemainingBurst(running_processes);
    }
    /* if there is a processes running and the number of processes is less or equal to allowed processes */
    if(running_processes > 0 && running_processes <= PID_COUNT){
      /* check if the current process running has finished */
      if (process[current_p].time_remaining == 0) {
        /* store completed time on process struct */
        process[current_p].completed = counter;
        /* find the next process with the lowest burst time */
        current_p = LowestRemainingBurst(running_processes);
      }
      /* decrement time remaining on the current running process */
      process[current_p].time_remaining--;
    }
    counter++;
  }
  /* print finish time results to terminal */
  ResultsToTerminal();

  /* collect average turnaround time and average wait time after scheduling finishes */
  double avgTurnaround = AverageTurnaroundTime();
  double avgWait = AverageWaitTime();

  /* lock thread when writing to fifo */
  pthread_mutex_lock(&mutex);

  /* create fifo with read and write permissions */
  int fifo = open(FIFO_PATH, O_CREAT | O_RDWR);
  if(fifo == -1)
    perror("Error opening FIFO.");
  else{
    /* write collected average wait and turnaround times to fifo */
    if (write(fifo, &avgTurnaround, sizeof(double)) == -1)
      perror("Error writing Average Turnaround Time to FIFO. ");
    if (write(fifo, &avgWait, sizeof(double)) == -1)
      perror("Error writing Average Wait Time to FIFO. ");
    printf("The processes have finished running.\n");
  };
  /* unlock thread */
  pthread_mutex_unlock(&mutex);
  /* signal to the write thread that srtf thread is done */
  sem_post(&write_semaphore);
}

/* Write thread.
*  Program average wait time and turnaround time is read from the pipe and written to file.
*/
void* WriteThread(){
  /* wait for write semaphore signal */
  sem_wait(&write_semaphore);
  double avgTurnaround, avgWait;

  printf("Writing average wait time and turnaround time to %s file...\n", OUT_FILE);
  /* lock thread when accessing to read from fifo */
  pthread_mutex_lock(&mutex);

  /* access fifo path with read only permissions; we don't want to accidentally write to it */
  int fifo = open(FIFO_PATH, O_RDONLY);
  if(fifo){
    /* read from fifo and store it in local function variables */
    if (read(fifo, &avgTurnaround, sizeof(double)) == -1)
      perror("Error reading Average Turnaround Time to FIFO.");
    if (read(fifo, &avgWait, sizeof(double)) == -1)
      perror("Error reading Average Wait Time to FIFO.");
    /* close fifo and unlink it to prevent errors on program rerun */
    if (close(fifo) == -1 || unlink(FIFO_PATH) == -1)
      perror("Error closing FIFO.");
    /* unlock thread */
    pthread_mutex_unlock(&mutex);
    /* oprn file with write permissions */
    if (!(file = fopen(OUT_FILE, "w")))
      perror("Error opening to file. ");
    else
      /* write collected results from fifo to file then close the file */
      fprintf(file, "Results\r\n Average Turnaround Time: %.4f ms\r\n Average Wait Time: %.4f ms", avgTurnaround, avgWait);
    fclose(file);
  }
  else
    perror("Error opening FIFO. ");
}
