//Christine Vinaviles 
//48450 Real-Time Operating Systems Assignment 2
//Date: 1st May 2018

// to compile gcc 11986282_Prg2.c -o prg_2.c -pthread -lrt
// to run the compiled file ./prg_2.c

//Included libraries
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

/* define shared memory object size*/
#define SIZE 4096
/* define buffer size */
#define BUFFER 100
/* define shared memeory name*/
#define shared_name "shared_memory"

/* shared memory file descriptor */
int shm_mem;
/* memory pointer */
void *mem_ptr;

/* Function to unlink shared memory.
*/
void UnlinkSharedMemory(){
  /* unlink the shared memory object */
  if (shm_unlink(shared_name) == 0)
    printf("Shared memory object unlinked!\n\nTerminating program...\n");
  else perror("Shared memory unlinking failed.");
}

/* Asks the user if they would like unlink the memory.
*/
void UserPrompt(){
	char user_input[BUFFER];

	while(1){
		printf("Would you like to unlink shared memory? [Y/N] ");
		scanf("%s[^\n]", user_input);

		/* check if user input is Y or N. If Y, unlink shared memory. */
		if (strstr(user_input, "Y") != 0){
       UnlinkSharedMemory();
       break;
     }
		/* check if user input is N then retain shared memory object. */
		else if (strstr(user_input, "N") != 0){
			printf("Retaining shared memory object. Terminating program...\n");
			break;
		}
		else printf("Please type a valid input. Y/N only.\n");
	}
}

/* Program main.
*/
int main(){
  /* check if there is a linked shared memory */
  if ((shm_mem = shm_open(shared_name, O_RDONLY, 0666)) != -1){
    /* memory map shared memory object */
    mem_ptr = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_mem, 0);

    /* read from the shared memory object */
    printf("Latest stored program 1 runtime: %s seconds.\n\n", (char *)mem_ptr);
    UserPrompt();
  }
  else printf("No linked shared memory, please run program 1 again.\nTerminating program...\n");

  return 0;
}
