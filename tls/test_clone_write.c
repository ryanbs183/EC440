#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

int tls_create(unsigned int size);
int tls_destroy();
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_clone(pthread_t target_tid);

sem_t* writing, *destroy;

void* create(void* args){
    tls_create(getpagesize());
    char w_buf[5] = "Hello";
    char r_buf[5];
    tls_write(0, 5, w_buf);
    tls_read(0, 5, r_buf);

    printf("CREATE READ BUF: %s\n", r_buf);
    printf("CREATE WRITE BUF: %s\n", w_buf);
    sem_post(writing);

    sem_wait(destroy);
    tls_destroy();

    return NULL;
}

void* clone(void* args){
    sem_wait(writing);
    tls_clone(*((pthread_t*)args));
    char w_buf[6] = " World";
    char r_buf[12];
    tls_write(5, 6, w_buf);
    tls_read(0,12, r_buf);

    printf("CLONE READ BUF: %s\n", r_buf);

    tls_destroy();

	sem_post(destroy);
    return NULL;
}

int main(int argc, char **argv){
    writing = (sem_t*) malloc(sizeof(sem_t));
	destroy = (sem_t*) malloc(sizeof(sem_t));
	sem_init(writing, 0, 0);
	sem_init(destroy, 0, 0);
    
    pthread_t t1, t2;

    pthread_create(&t1, NULL, &create, NULL);
    pthread_create(&t2, NULL, &clone, &t1);

    pthread_join(t2, NULL);

    sem_destroy(writing);
	sem_destroy(destroy);
    printf("Done...");

    return 0;
}