#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>

sem_t empty;
sem_t full;
sem_t mutex;
int items;

void* prod(void *args){
    while(1){
        sem_wait(&empty);
        sem_wait(&mutex);

        printf("%d: Producing...\n", (int)pthread_self());
        items++;

        sem_post(&mutex);
        sem_post(&full);
    }
}

void* cons(void *args){
    while(1){
        sem_wait(&full);
        sem_wait(&mutex);

        printf("%d: Consuming...\n", (int)pthread_self());
        items--;

        sem_post(&mutex);
        sem_post(&empty);
    }
}

int main(){
    pthread_t threads[3];
    sem_init(&full, 0, 0);
    sem_init(&empty, 0, 10);
    sem_init(&mutex, 0, 1);
    items = 0;

    pthread_create(&threads[0], NULL, prod, NULL);
    pthread_create(&threads[1], NULL, cons, NULL);
    pthread_create(&threads[2], NULL, cons, NULL);

    while(1){
        sem_wait(&full);
        sem_wait(&mutex);

        printf("%d: Consuming...\n", (int)pthread_self());
        items--;

        sem_post(&mutex);
        sem_post(&empty);
    }
    return 1;
}