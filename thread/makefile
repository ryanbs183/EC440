threads: threads.c
	gcc  threads.c -c -Wall -Werror -g -o threads.o

test:
	gcc test-threads.c threads.o -g -Wall -o test

clean:
	rm threads.o
	rm test
