#ifndef THREADS_H
#define THREADS_H

#include <pthread.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include "ec440threads.h"


#define MAXTHREAD	128
#define INTERVAL	50
#define STACKMEM	32767
#define JB_RBX  0
#define JB_RBP  1
#define JB_R12  2
#define JB_R13  3
#define JB_R14  4
#define JB_R15  5
#define JB_RSP  6
#define JB_PC   7

#define READY 0
#define RUNNING 1
#define EXIT 2
#define PAUSED 3
#define BLOCKED 4

/* thread control block */
typedef struct tcb{
	pthread_t id;           // registers the thread id
    struct tcb *next;       // tracks next thread to be executed
    struct tcb *last;       // tracks last thread to be executed
    jmp_buf buf;            // stores the register values
    void *stack_ptr;        // points to threads space on the stack
    int state;              // keeps track of thread state: 0 = READY, 1 = RUNNING, 2 = EXIT, 3 = PAUSED, 4 = BLOCKED
    void* ret_val;          // stores thread return value on exit
    pthread_t tgt_thread;    // stores the id of the blocking thread
} nthread;

/* keeps track of the threads controlled by the semaphore */
typedef struct thread_queue{
    nthread* t;
    struct thread_queue* next;
}tq;

/* stores semaphore variables and controlled threads */
typedef struct sem_struct{
    int init;
    int val;
    tq* head;
}sema;

/* threading functions */
pthread_t pthread_self(void);
void pthread_exit(void *value_ptr);
int pthread_create(pthread_t *thread,const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);
int pthread_join(pthread_t thread, void **value_ptr);

/* semaphore functions */
int sem_init(sem_t *sem, int pshared, unsigned value);
int sem_wait(sem_t *sem);
int sem_post(sem_t *sem);
int sem_destroy(sem_t *sem);

/* helper functions */
int init();
void schedule();
void lock();
void unlock();
void pthread_exit_wrapper();
nthread* thread_exists(nthread* start, pthread_t thread);

/* ASM functions for editing registers */
void wr_reg12(unsigned long int p);
void wr_reg13(unsigned long int p);


#endif