/*
(1) TerrierID: terrier089
================================================================================
(2) Target Characteristic To Test:
--------------------------------------------------------------------------------
Test whether a value and be written to and read back from TLS.
================================================================================
(3) Justification for WHY Target Charactersitic is Valuable to Test
--------------------------------------------------------------------------------
This test verifies that the TLS implementation can load and store data.
================================================================================
(4) Sketch HOW You Would Implement the Testcase
--------------------------------------------------------------------------------
Create a thread that writes a value to TLS and ensures that the same value can
be read back.
================================================================================
(5) All tests must build as:
gcc -Werror -Wall -o main -lpthread <filename.c>
*/
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

int tls_create(unsigned int size);
int tls_destroy();
int tls_read(unsigned int offset, unsigned int length, char *buffer);
int tls_write(unsigned int offset, unsigned int length, char *buffer);
int tls_clone(pthread_t target_tid);

#define TEST_ASSERT(x) do { \
	if (!(x)) { \
		fprintf(stderr, "%s:%d: Assertion (%s) failed!\n", \
				__FILE__, __LINE__, #x); \
	       	exit(1); \
	} \
} while(0)

void*
my_thread(void *arg)
{
	int written_int = 10;
	int read_int = 0;

	TEST_ASSERT(tls_create(sizeof(int)) == 0);

	TEST_ASSERT(tls_write(0, sizeof(int), (char*)&written_int) == 0);
	TEST_ASSERT(tls_read(0, sizeof(int), (char*)&read_int) == 0);
	TEST_ASSERT(read_int == written_int);

	TEST_ASSERT(tls_destroy() == 0);
	return NULL;
}

int
main(void)
{
	pthread_t tid;
	TEST_ASSERT(pthread_create(&tid, NULL, my_thread, NULL) == 0);
	TEST_ASSERT(pthread_join(tid, NULL) == 0);
	return 0;
}
