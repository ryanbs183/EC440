#include "tls.h"

/*
    Ryan Sullivan
    U73687595
*/
#define HASH_SIZE 128

// global variables //
hash* hash_table[HASH_SIZE];
int initialized = 0;
int page_size;
int tls_count = 0;

int tls_create(unsigned int size){
    if( size <= 0 || find_tls(pthread_self()) != NULL){
        return -1;
    }
    if(!initialized){
        tls_init();
    }
    // allocate structure memory and get the current thread id
    pthread_t thread_id = pthread_self();
    tls *new_tls = malloc(sizeof(tls));
    hash *new_element = malloc(sizeof(hash));

    // initialize new tls
    new_tls -> mem_size = size;
    new_tls -> pg_num = (size % page_size == 0) ? size/(page_size) : size/(page_size) + 1;
    new_tls -> pages = malloc((new_tls -> pg_num)*sizeof(page*));
    new_tls -> tid = thread_id;

    int i;
    for(i = 0; i < new_tls -> pg_num; i++){
        page* new_pg = malloc(sizeof(page));
        new_pg -> addr = (unsigned long int)(mmap(0, page_size, 0, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0));
        new_pg -> ref_count = 1;
        tls_protect(new_pg);
        new_tls -> pages[i] = new_pg;
    }

    // add new tls to the hash table
    new_element -> tid = thread_id;
    new_element -> t_storage = new_tls;
    insert_hash(new_element);
    tls_count++;
    return 0;
}

int tls_write(unsigned int offset, unsigned int length, char *buffer){
    // error checking //
    hash* curr_tls = find_tls(pthread_self());
    if(curr_tls == NULL || ((offset + length) > (curr_tls -> t_storage -> mem_size))){
        return -1;
    }
    
    // unprotect memory //
    int i;
    for(i = 0; i < curr_tls -> t_storage -> pg_num; i++){
        tls_unprotect(curr_tls -> t_storage -> pages[i]);
    }

    unsigned int cnt, idx;
    for(cnt = 0, idx = offset; idx < (offset + length); ++idx, ++cnt){
        page *p;
        page *cpy;
        unsigned int pn, poff;
        pn = idx / page_size;
        poff = idx % page_size;
        p = curr_tls -> t_storage -> pages[pn];
        if(p -> ref_count > 1){
            cpy = (page*)malloc(sizeof(page));
            cpy -> addr = (unsigned long int)mmap(0, page_size, PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
            cpy -> ref_count = 1;
            memcpy((void*)(cpy -> addr), (void*)(p -> addr), page_size);
            curr_tls -> t_storage -> pages[pn] = cpy;

            p -> ref_count--;
            tls_protect(p);
            p = cpy;
        }
        char *dst = ((char *) p -> addr) + poff;
        *dst = buffer[cnt];
    }
    // reprotect memory //
    for(i = 0; i < curr_tls -> t_storage -> pg_num; i++){
        tls_protect(curr_tls -> t_storage -> pages[i]);
    }
    return 0;
}
int tls_read(unsigned int offset, unsigned int length, char *buffer){
    hash* curr_tls = find_tls(pthread_self());
    if(curr_tls == NULL || ((offset + length) > (curr_tls -> t_storage -> mem_size))){
        return -1;
    }
    // unprotect memory //
    int i;
    for(i = 0; i < curr_tls -> t_storage -> pg_num; i++){
        tls_unprotect(curr_tls -> t_storage -> pages[i]);
    }

    unsigned int cnt, idx;
    for (cnt = 0, idx = offset; idx < (offset + length); ++cnt, ++idx) {
        page *p;
        unsigned int pn, poff;

        pn = idx / page_size;
        poff = idx % page_size;

        p = curr_tls -> t_storage -> pages[pn];
        char *src = ((char *) p->addr) + poff;
        buffer[cnt] = *src;
    }
    // reprotect memory //
    for(i = 0; i < curr_tls -> t_storage -> pg_num; i++){
        tls_protect(curr_tls -> t_storage -> pages[i]);
    }

    return 0;
}

int tls_destroy(){
    hash* curr_tls = find_tls(pthread_self());
    if(curr_tls == NULL){
        return -1;
    }
    int i;
    for(i = 0; i < (curr_tls -> t_storage -> pg_num); i++){
        if(curr_tls -> t_storage -> pages[i] -> ref_count <= 1) {
            munmap((void*)(curr_tls -> t_storage -> pages[i] -> addr), page_size);
            free(curr_tls -> t_storage -> pages[i]);
        } else {
            curr_tls -> t_storage -> pages[i] -> ref_count--;
        }
    }

    free(curr_tls -> t_storage -> pages);
    free(curr_tls -> t_storage);
    int index = (int)(curr_tls -> tid) % HASH_SIZE;
    if(hash_table[index] == curr_tls){
        hash_table[index] = NULL;
    } else {
        int i = (int)(curr_tls -> tid) % HASH_SIZE;
        while(hash_table[i] == NULL || hash_table[i] -> tid != curr_tls -> tid){
            i = (i+1) % HASH_SIZE;
        }
        hash_table[i] = NULL;
    }
    free(curr_tls);
    tls_count--;
    return 0;
}

int tls_clone(pthread_t tid){
    hash* cpy_tls = find_tls(tid);
    if(find_tls(pthread_self()) != NULL || cpy_tls == NULL) return -1;

    hash* new_element = malloc(sizeof(hash));
    tls* new_tls = malloc(sizeof(tls));
    
    new_tls -> tid = pthread_self();
    new_tls -> mem_size = cpy_tls -> t_storage -> mem_size;
    new_tls -> pg_num = cpy_tls -> t_storage -> pg_num;
    new_tls -> pages = malloc((new_tls -> pg_num)*sizeof(page*));
    int i;
    for(i = 0; i < new_tls -> pg_num; i++){
        new_tls -> pages[i] = cpy_tls -> t_storage -> pages[i];
        new_tls -> pages[i] -> ref_count++;
    }

    new_element -> tid = new_tls -> tid;
    new_element -> t_storage = new_tls;
    insert_hash(new_element);
    tls_count++;
    return 0;
}

void tls_init(){
    int i;
    for(i = 0; i < HASH_SIZE; i++){
        hash_table[i] = NULL;
    }
    struct sigaction sigact;
    /* get the size of a page */
    page_size = getpagesize();
    /* install the signal handler for page faults (SIGSEGV, SIGBUS) */
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_SIGINFO; /* use extended signal handling */
    sigact.sa_sigaction = handle_seg;
    sigaction(SIGBUS, &sigact, NULL);
    sigaction(SIGSEGV, &sigact, NULL);
    initialized = 1;
    tls_count++;
}

void handle_seg(int sig_code, siginfo_t* info, void *context){
    unsigned long int p_fault = ((unsigned long int) info->si_addr) & ~(page_size - 1);
    int i;
    for(i = 0; i < HASH_SIZE; i++){
        int j;
        for(j = 0; j < hash_table[i] -> t_storage -> pg_num; j++){
            if(hash_table[i] -> t_storage -> pages[j] -> addr == p_fault){
                pthread_exit(NULL);
            }
        }
    }

    signal(SIGSEGV, SIG_DFL);
    signal(SIGBUS, SIG_DFL);
    raise(sig_code);
}

void tls_unprotect(page *p){
    if (mprotect((void *) p->addr, page_size, PROT_READ | PROT_WRITE)) {
        fprintf(stderr, "tls_unprotect: could not unprotect page\n");
        exit(1);
    }
}

void tls_protect(page *p){
    if (mprotect((void *) p->addr, page_size, PROT_NONE)) {
        fprintf(stderr, "tls_protect: could not unprotect page\n");
        exit(1);
    }
}

hash* find_tls(pthread_t thread){
    int i = (int) thread % HASH_SIZE;
    if(hash_table[i] != NULL && hash_table[i] -> tid == thread){ 
        return hash_table[i];
    } else {
        int start = i;
        while(hash_table[i] == NULL || hash_table[i] -> tid != thread){
            if(start == (++i % HASH_SIZE)){
                break;
            }
        }
        i = i %HASH_SIZE;
        if(start != i){
            return hash_table[i];
        }
    }
    return NULL;
}

void insert_hash(hash* el){
    int index = ((int)(el -> t_storage -> tid)) % HASH_SIZE;
    if(hash_table[index] == NULL){
        hash_table[index] = el;
    } else {
        hash* current = hash_table[(++index) % HASH_SIZE];
        while(current != NULL){
            current = hash_table[(++index) % HASH_SIZE];
        }
        hash_table[index] = el;
    }
}
