threads: threads.c
	gcc  threads.c -c -Wall -Werror -g -o threads.o

test:
	gcc test-threads.c threads.o -g -Wall -o test
	gcc sema_test.c threads.o -g -Wall -o test_sem

clean:
	rm threads.o
	rm test
	rm test_sem
