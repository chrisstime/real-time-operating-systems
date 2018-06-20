//Christine Vinaviles 11986282
//48450 Real-Time Operating Systems Assignment 3
//Date: 22nd May 2018

// to compile gcc 11986282_A3_Prg2.c -o prg_2.c
// to run the compiled file ./prg_2.c

/* Include required libraries for the program to run */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

/* Static variable declarations */
#define PID_COUNT 9
#define RESOURCE_COUNT 3
#define BUFFER_SIZE 100
#define SRC_FILE "data_file.txt"
#define OUT_FILE "prg_output.txt"

/* process struct. Contains variables required per process */
typedef struct{
  char p_id[BUFFER_SIZE];
  int allocation[RESOURCE_COUNT];
  int request[RESOURCE_COUNT];
  int finished;
} process_struct;

/* initialise 9 instances of process struct */
process_struct process[PID_COUNT];
/* work / avialable variable */
int work[RESOURCE_COUNT];
/* array to store safe sequence */
int sequence[PID_COUNT];

/* Method to handle initial user input to run the program.
*/
int StartMenu(){
	int exitProgram = 0;
	char user_input[BUFFER_SIZE];

	printf("Program starting...\n");
	while(1){
		printf("Would you like to proceed? [y/n] ");
		scanf("%s[^\n]", user_input);

		/* check if user input is Y or N*/
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

/* Function to read from Topic2_Prg_2.txt file.
*  Returns 0 on success
*/
int ReadProgramFile(){
  FILE* file;
  int i = 0;

  /* open file with read permissions */
  if (!(file = fopen(SRC_FILE, "r"))){
    perror("Error opening to file. ");
    return 1;
  }
  else{
    /* scan and discard to ignore the first two lines of the file. */
    fscanf(file, "%*[^\n]\n%*c %*[^\n]%*c");

    /* run while end of file has not been reached and index is less than number of processes */
    while (!feof(file) && i < PID_COUNT){
      /* scan items in the file and store them in the appropriate process struct variables */
      fscanf(file, "%s %d %d %d %d %d %d %d %d %d",
        &process[i].p_id[0],
        &process[i].allocation[0], &process[i].allocation[1], &process[i].allocation[2],
        &process[i].request[0], &process[i].request[1], &process[i].request[2],
        &work[0], &work[1], &work[2]
      );
      i++;
    }
    /* Close file */
    if (fclose(file) != 0){
      perror("Error closing file. ");
      return 1;
    }
  }
  return 0;
}

/* Function to print the scanned contents from the source file to the terminal.
*  Useful for checking that the variables have been scanned correctly.
*/
void OutputToTerminal(){
  int i;
  /* print all variables to terminal. These should match the orinal source file. */
  printf("Available: %d%d%d\n", work[0], work[1], work[2]);

  for (i = 0; i < PID_COUNT ; i++){
    printf("PID: %s  Allocation: %d%d%d  Request: %d%d%d\n",
            process[i].p_id,
            process[i].allocation[0], process[i].allocation[1], process[i].allocation[2],
            process[i].request[0], process[i].request[1], process[i].request[2]
          );
  }
}

/* Function to detect any deadlock using Banker's algorithm.
*/
void DeadlockDetection(){
  int i, j;
  int sequence_index = 0;

  /* loop through all the processes first */
  for(i = 0; i < PID_COUNT; i++){
    /* check the current process has any allocated instances of resource */
    if(process[i].allocation[0] == 0
      && process[i].allocation[1] == 0
      && process[i].allocation[2] == 0)
      /* if they don't then set the process as finished */
      process[i].finished = 0;
    /* otherwise, set them as not finished */
    else process[i].finished = -1;
  }

  /* Banker's algorithm. Continuously loop through all the processes until
  *  all processes are sequenced
  */
  while(sequence_index < PID_COUNT){
    for(i = 0; i < PID_COUNT; i++)
      /* if process isn't finished */
      if(process[i].finished == -1)
        /* and Request <= work for all allocated resource in current process */
        if(process[i].request[0] <= work[0] &&
           process[i].request[1] <= work[1] &&
           process[i].request[2] <= work[2]){
          /* then new work value = work + allocation */
          for (j = 0; j < RESOURCE_COUNT; j++)
            work[j] += process[i].allocation[0];
          /* indicate that this process is now finished */
          process[i].finished = 0;
          /* increment sequence index */
          sequence[sequence_index++] = i;
        }
  }
}

/* Function to write the deadlock detection function results to file.
*/
void WriteResultsToFile(){
  FILE* file;
  int i;
  int deadlock_count = 0;
  int deadlocks[PID_COUNT];
  /* open file with write permissions */
  if (!(file = fopen(OUT_FILE, "w"))){
    perror("Error opening to file. ");
  }
  else{
    printf("Writing deadlock detection results to %s..\n", OUT_FILE);
    /* loop through the process structs and if any of them are not finished */
    for(i = 0; i < PID_COUNT; i++){
      if (process[i].finished != 0){
        deadlocks[deadlock_count++] = i;
        /* print any unfinished process to file as deadlocked */
        fprintf(file, "Deadlock occurs at PID %d\r\n", deadlocks[i]);
      }
    }
    /* if there are no deadlocks */
    if(deadlock_count == 0){
      fprintf(file, "Program safe sequence:\r\n < ");
      /* loop through and print out the recorded safe sequence based on
         Banker's algorithm */
      for(i = 0 ; i < PID_COUNT; i++)
        fprintf(file, "P%d ", sequence[i]);
      fprintf(file, ">");
    }
    /* Close the file */
    if (fclose(file) != 0) perror("Error closing file. ");
  }
  /* if signal wasn't raised on program run. Flag as error */
  if (raise(SIGUSR1) != 0) perror("Signal raise error. ");
}

/* C99 function for handling signals. This one is to signal was raised
*  to indicate the program has completed running.
*/
void sighandler(int sig){
  printf("Write to file success.\n");
}

int main(void){
  if (StartMenu() != 1){
    /* initialise signal */
    signal(SIGUSR1, sighandler);
    /* if reading from the source file was a success run all the other functions */
    if (ReadProgramFile() == 0){
      OutputToTerminal();
      DeadlockDetection();
      WriteResultsToFile();
    }
		printf("Program finished. Terminating...\n");
	}
  exit(0);
}
