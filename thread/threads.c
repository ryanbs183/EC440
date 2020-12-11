#include "threads.h"

/* 
	Ryan Sullivan
	U73687595
*/

/* global variables */
nthread *currThread = NULL;		// keeps track of currently executing thread
int thread_id = 0;				// current thread id
int numThreads = 0;				// keeps track of the number of currently active threads

/* create a new thread and add it to the queue */
int pthread_create(pthread_t *thread,const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg){
	lock();
	if (numThreads == 0) {
        init();
    } else if (numThreads >= MAXTHREAD) {
        printf("Too many threads, try again later!!\n");
		return 1;
    }
	// create newThread
    nthread *newThread = malloc(sizeof(nthread));

	// allocate thread stack memory
    newThread -> stack_ptr = malloc(STACKMEM);

	// set spot in memory for pthread_exit_wrapper
    unsigned long int *exit = (unsigned long int *)(newThread -> stack_ptr + (STACKMEM/sizeof(unsigned long int) - 1));	// point to top of stack
    *exit = (unsigned long int)pthread_exit_wrapper;	// place pthread_exit_wrapper at the top of the stack

	// initialize newThread
    newThread -> state = READY;
    newThread -> id = (pthread_t)thread_id;
	newThread -> ret_val = NULL;
	newThread -> tgt_thread = (pthread_t)-1;
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
	unlock();
    return 0;
}

int pthread_join(pthread_t thread, void **value_ptr){
	// check for target thread and block current thread
	nthread* target = thread_exists(currThread, thread);
	if(target == NULL || thread == currThread -> id){
		return 1;
	}
	lock();
	currThread -> state = BLOCKED;
	currThread -> tgt_thread = thread;

	if(target -> state != EXIT){
		unlock();

		schedule();
		unlock();

		lock();
	} else{
		currThread -> state = READY;
	}

	if(value_ptr != NULL){
		nthread* step = currThread -> next;
		nthread* start = currThread;
		while(step != start){
			if(step -> id == thread){
				*value_ptr = step -> ret_val;
				break;
			}
			step = step -> next;
		}
	}
	unlock();
	return 0;
}

/* check to see if thread exists and return its state */
nthread* thread_exists(nthread* start, pthread_t thread){
	nthread *step = start -> next;
	while(step != start){
		if(step -> id == thread){
			return step;
		}
		step = step -> next;
	}
	return NULL;
}

/* exit current thread and free stack */
void pthread_exit(void *value_ptr){
	lock();
	currThread -> state = EXIT;
	currThread -> ret_val = value_ptr;
	free(currThread -> stack_ptr);
	numThreads--;
	unlock();
	schedule();
	unlock();
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
	initThread -> stack_ptr = malloc(STACKMEM);
	initThread -> ret_val = NULL;
	initThread -> state = READY;
	initThread -> tgt_thread = (pthread_t)-1;
	numThreads++;
	thread_id++;
	setjmp(initThread -> buf);

	signal(SIGALRM, schedule);
	ualarm(INTERVAL*1000, INTERVAL*1000);

	currThread = initThread;

	return 0;
}

/* pause current thread and jump to next thread */
void schedule(){
	// check to see if uninitialized
	if(currThread == NULL){
		return;
	}
	lock();
	// save state of current thread 
	if(setjmp(currThread -> buf) == 0){
		// check to make sure schedule is not called after exit or block
		if(currThread -> state != EXIT && currThread -> state != BLOCKED && currThread -> state != PAUSED){
			currThread -> state = READY;	// set thread to READY
		}
		currThread = currThread -> next;	// move to next thread in line

		// keep stepping until thread is no longer blocked or exited
		while(currThread -> state == BLOCKED|| currThread -> state == EXIT || currThread -> state == PAUSED){
			if(currThread -> state == BLOCKED){
				nthread *tgt = thread_exists(currThread, currThread -> tgt_thread);
				if(tgt != NULL && tgt -> state == EXIT){
					break;
				}
			}
			currThread = currThread -> next;
		}
		currThread -> state = RUNNING;		// set new thread to RUNNING
		longjmp(currThread -> buf, 1); 		// jump to new thread execution
	}
}

/* wrapper function for pthread_exit to allow for saving of return value of thread */
void pthread_exit_wrapper() {
	unsigned long int res;
	asm("movq %%rax, %0\n":"=r"(res));
	pthread_exit((void *) res);
}

/* disable interactivity of calling thread */
void lock(){
	sigset_t blocked_sig;
	sigemptyset(&blocked_sig);
	sigaddset(&blocked_sig, SIGALRM);

	sigprocmask(SIG_BLOCK, &blocked_sig, NULL);
}

/* reenable interactivity of calling thread */
void unlock(){
	sigset_t unblocked_sig;
	sigemptyset(&unblocked_sig);
	sigaddset(&unblocked_sig, SIGALRM);

	sigprocmask(SIG_UNBLOCK, &unblocked_sig, NULL);
}

/* write p to r12 */
void wr_reg12(unsigned long int p){
	asm("movq %0, %%r12;": : "r"(p));
}

/* write p to r13 */
void wr_reg13(unsigned long int p){
	asm("movq %0, %%r13;": : "r"(p));
}

/* semaphore functions  */

int sem_init(sem_t *sem, int pshared, unsigned value){
	lock();
	sema* initSem = malloc(sizeof(sema));
	initSem -> val = value;
	initSem -> init = 1;
	initSem -> head = NULL;
	sem -> __align = (long int)initSem;
	unlock();
	return 0;
}

int sem_wait(sem_t *sem){
	lock();
	sema* checkSem = (sema*)(sem -> __align);
	if(checkSem -> val == 0){
		if(checkSem -> head == NULL){
			checkSem -> head = malloc(sizeof(tq));
			checkSem -> head -> next = NULL;
			checkSem -> head -> t = currThread;
		} else {
			tq* step = checkSem -> head;
			while(step -> next != NULL){
				step = step -> next;
			}
			step -> next = malloc(sizeof(tq));
			step -> next -> next = NULL;
			step -> next -> t = currThread;
		}
		currThread -> state = PAUSED;
		unlock();
		schedule();
	}
	--(checkSem -> val);
	unlock();
	return 0;
}

int sem_post(sem_t *sem){
	lock();
	sema* checkSem = (sema*)(sem -> __align);
	
	if(checkSem -> head != NULL){
		checkSem -> head -> t -> state = READY;
		checkSem -> head = checkSem -> head -> next;
		free(temp);
	}

	++(checkSem -> val);
	unlock();
	return 0;
}

int sem_destroy(sem_t *sem){
	lock();
	sema* deleteSum = (sema*)(sem -> __align);
	if(deleteSum -> head != NULL){
		tq* step = deleteSum -> head;
		while(step -> next != NULL){
			tq* temp = step -> next;
			free(step);
			step = temp;
		}
		free(step);
	}

	free(deleteSum);
	unlock();
	return 0;
}