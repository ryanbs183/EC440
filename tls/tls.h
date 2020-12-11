#ifndef TLS_H
#define TLS_H

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

// variable type definitions //
typedef struct page_object {
    unsigned long int addr;
    int ref_count;
} page;

typedef struct thread_local_storage {
    pthread_t tid;
    unsigned int mem_size;
    unsigned int pg_num;
    page** pages;
} tls;

typedef struct hash_element {
    pthread_t tid;
    tls* t_storage;
    struct hash_element *next;
} hash; 

// main function definitions //
int tls_create(unsigned int size);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_destroy();
int tls_clone(pthread_t tid);

// helper function definitions //
void tls_init();
void handle_seg(int sig_code, siginfo_t* info, void *context);
void tls_unprotect(page *p);
void tls_protect(page *p);
hash* find_tls(pthread_t tid);
void insert_hash(hash* el);

#endif