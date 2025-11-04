.PHONY: test
test: libpj.h test.c
	gcc -Wall -Wextra -x c test.c -o test
	./test
