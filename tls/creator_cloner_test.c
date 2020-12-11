/*
(1) TerrierID: terrier007
================================================================================
(2) Target Characteristic To Test:
--------------------------------------------------------------------------------
Tests whether tls_clone functions to clone an LSA without CoW being tested (everything is 
written before it is cloned).
Additionally test tls_destroy functionality for a shared LSA.(100 Words max)
================================================================================
(3) Justification for WHY Target Charactersitic is Valuable to Test
--------------------------------------------------------------------------------
It is useful to check that tls clone can properly clone another threads LSA and read from that LSA.
(250 Words max)
================================================================================
(4) Sketch HOW You Would Implement the Testcase
--------------------------------------------------------------------------------
This test case is implemented so that the creator thread makes a new LSA for itself
which it then writes to.  The cloner thread is created, but waits for the creator thread
to finish writing to the LSA.  Then the cloner thread reads from the LSA and prints each page
which should be totally comprised of the page number.  Then the cloner thread destroys the LSA.
 (250 Words max)
================================================================================
(5) All tests must build as:
gcc -Werror -Wall -o main -lpthread <filename.c>
*/
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>

int tls_create(unsigned int size);
int tls_destroy();
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_clone(pthread_t target_tid);

#define PAGE_NUM 2

//Defines semaphores needed to perform the given functionality
sem_t* writing, *destroy;

//Create a TLS of size pageNumber and then initializes its contents
void *creator(void *arg)
{
	int pageSize = getpagesize();

	tls_create(pageSize * PAGE_NUM);
	
	//Loop to add a int to each page
	int i, j;
	char str [pageSize];
	for (i = 0; i < PAGE_NUM; i++)
	{
		//Create a string of size equal to that of a page which contains the current pageNumber repeated
		for (j = 0; j < pageSize; j++)
		{
			str[j] = (char) i + '0';
		}
		printf("Page %d's string is: %s\n", i, str);

		//Write that string to the appropriate page
		tls_write(i*pageSize, pageSize, str);
	}

	//Allow cloner to clone
	sem_post(writing);

	//Wait to destroy until cloner has done so
	sem_wait(destroy);
	tls_destroy();

	return NULL;
}

//Clones the contents of the creator's LSA and then reads from them
void *cloner(void *arg)
{
	int pageSize = getpagesize();
	
	//Translate arg to pthread_t
	pthread_t creatorTID = *((pthread_t*) arg);

	//Wait for first thread to finish writing to LSA
	sem_wait(writing);

	//Clone the TLS of creator
	tls_clone(creatorTID);

	//Loop to read each string from each page of cloned LSA
	int i;
	for (i = 0; i < PAGE_NUM; i++)
	{
		//Create a string of size equal to that of a page which contains the current pageNumber repeated
		char str [pageSize];
		
		//Write that string to the appropriate page
		tls_read(i*pageSize, pageSize, str);

		//Print each page of the LSA
		printf("Page %d: %s\n", i, str);
	}

	tls_destroy();

	sem_post(destroy);

	return NULL;
}

int main(int argc, char **argv) {
	pthread_t t1, t2;

	//Initialize needed semaphores
	writing = (sem_t*) malloc(sizeof(sem_t));
	destroy = (sem_t*) malloc(sizeof(sem_t));
	sem_init(writing, 0, 0);
	sem_init(destroy, 0, 0);

	//Creates first thread which intializes the LSA
	pthread_create(&t1, NULL, &creator, NULL);

	//Creates second thread which clones the LSA and then reads it
	pthread_create(&t2, NULL, &cloner, &t1);

	//Wait for second thread to exit
	pthread_join(t2, NULL);

	//Destroy the semaphores
	sem_destroy(writing);
	sem_destroy(destroy);

	printf("done ...\n");	
	return 0;
}
