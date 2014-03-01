PROJECT = cs550p1_chen_liehuo
SRC_FILES = \
	malloc.c \
	test.c \
	Makefile \
	README.txt

test.out:test.c malloc.c
	gcc -Wall -Wextra $^ -o $@

clean:
	rm -rf *.out *.o 

submit: $(SRC_FILES)
	tar cvfz $(PROJECT).tar.gz $(SRC_FILES)
