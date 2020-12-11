#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ec440threads.h"
#include "threads.h"

/* 
	Ryan Sullivan
	U73687595
*/

/* global variables */
nthread *currThread = NULL;		// keeps track of currently executing thread
int thread_id = 0;
int numThreads = 0;

/* function declarations */

int pthread_create(pthread_t *thread,const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg){
	if (numThreads == 0) {
        init();
    } else if (numThreads >= MAXTHREAD) {
        printf("Too many threads, try again later!!\n");
		return -1;
    }
	// create newThread
    nthread *newThread = malloc(sizeof(nthread));
    thread = (pthread_t *)newThread;

	// allocate thread stack memory
    newThread -> stack_ptr = malloc(STACKMEM);

	// set spot in memory for pthread_exit
    unsigned long int *exit = (unsigned long int *) newThread -> stack_ptr + (STACKMEM/sizeof(unsigned long int) - 1);	// point to top of stack
    *exit = (unsigned long int)pthread_exit;	// place pthread_exit at the top of the stack

	// initialize newThread
    newThread -> state = 0;
    newThread -> id = (pthread_t)thread_id;
    *thread = newThread -> id; 
    newThread -> last = currThread -> last;
    newThread -> next = currThread;
    currThread -> last -> next = newThread;
    currThread -> last = newThread;

	// write start_routine to reg12 and arg to reg13
    wr_reg12((unsigned long int)start_routine);
    wr_reg13((unsigned long int)arg);

	// store current state
    setjmp(newThread -> buf);
	
	// place start_thunk on the PC to run after longjmp
    newThread -> buf[0].__jmpbuf[JB_PC] = ptr_mangle((unsigned long int)start_thunk);
	// set the exit function as the return stack ptr
    newThread -> buf[0].__jmpbuf[JB_RSP] = ptr_mangle((unsigned long int)exit);

	// increment numThreads and thread_id
    numThreads++;
    thread_id++;
    return 0;
}

/* exit current thread and free stack */
void pthread_exit(void *value_ptr){
	currThread -> state = 2;
	currThread -> last -> next = currThread -> next;
	currThread -> next -> last =  currThread -> last;
	free(currThread -> stack_ptr);
	numThreads--;
	schedule();
	__builtin_unreachable();
}

/* return the current thread's id */
pthread_t pthread_self(){
	if(numThreads == 0){
		return (pthread_t)NULL;
	} else {
		return currThread -> id;
	}
}

/* initialize the timer interrupt and initial thread */
int init(){
	nthread *initThread = malloc(sizeof(nthread));
	initThread -> id = (pthread_t)thread_id;
	initThread -> last = initThread;
	initThread -> next = initThread;
	initThread -> stack_ptr = NULL;
	initThread -> state = 0;
	numThreads++;
	thread_id++;
	setjmp(initThread -> buf);

	signal(SIGALRM, schedule);
	ualarm(INTERVAL*1000, INTERVAL*1000);

	currThread = initThread;

	return 1;
}

/* pause current thread and jump to next thread */
void schedule(){
	/* check to see if uninitialized */
	if(currThread == NULL){
		return;
	}
	if(setjmp(currThread -> buf) == 0){
		/* check to make sure schedule is not called after exit */
		if(currThread -> state != 2){
			currThread -> state = 3;			// set thread to PAUSE
		}
		currThread = currThread -> next;	// move to next thread in line
		currThread -> state = 1;			// set new thread to RUNNING
		longjmp(currThread -> buf, 1); 		// jump to new thread execution
	}
}

/* write p to r12 */
void wr_reg12(unsigned long int p){
	asm("movq %0, %%r12;": : "r"(p));
}

/* write p to r13 */
void wr_reg13(unsigned long int p){
	asm("movq %0, %%r13;": : "r"(p));
}