/*
 * Small test program to test malloc.c 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>


//#define SIZE 30
//#define MAXN 1000

typedef struct {
	void* pt;
	int size;
	int value;
}Data;

int memcheck(Data* pd) {
	int i;
	char* p;
	for (i=0; i<pd->size; i++) {
		p = pd->pt + i;
		if (*p != (char)pd->value) {
			printf("Array content %d doesn't match the value %d\n!!!", (int)*p, pd->value);
			return 1;
		}
	}
	return 0;
}



int main(int argc, char** argv) {
	int n = 0, size,index, value;
	int gap, SIZE=28, MAXN=1000;
	struct timeval start, end;
	Data *arr;
	if (argc <= 2) {
		printf("Usage:  ./malloc MAX_MEMORY  ITERATIONS\n");
		printf("MAX_MEMORY stands for the log2(max size of memory) you want to allocate.\n");
		printf("ITERATIONS stands for the iteration times of the 4 operations you want to test.\n");
		printf("\n");
		printf("Note that it might exceed MAX memory when using calloc(block size 2)!\n");
		printf("So choose your MAX memory between 1 and 29 if you are using 32bit machine!\n");
		exit(1);
	}
	
	SIZE = atoi(argv[1]);
	MAXN = atoi(argv[2]);
	printf("Log2(MAX memory size) = %d, Loop times = %d\n", SIZE, MAXN);
	arr = malloc(SIZE*sizeof(Data));	
	memset(arr, 0, sizeof(Data)*SIZE);

	n = 0;

	gettimeofday(&start, NULL);
	srand(time(NULL));
	while(n<MAXN) {
		index = rand()%SIZE;
		value = rand()%10;
		size = rand()%(1<<index);
		printf("index =%d, value = %d, size = %d\n", index, value, size);
		if (arr[index].pt) {
			if (index%2) {
				free(arr[index].pt);
				arr[index].pt = NULL;
			} else {
				arr[index].pt = realloc(arr[index].pt, size);
				arr[index].size = size;
				arr[index].value = value;
				memset(arr[index].pt, value, size);
			}
		} else {
			if (index%2) {
				arr[index].pt = malloc(size);
				arr[index].size = size;
				arr[index].value = value;
				memset(arr[index].pt, value, size);	
			} else {
				arr[index].pt = calloc(2, size);
				arr[index].size = size*2;
				arr[index].value = value;
				memset(arr[index].pt, value, size*2);	
			}
		}
		n++;
	}

	for (n=0; n<SIZE; n++) {
		if (arr[n].pt) {
			if (memcheck(&arr[n]))
				printf("Warning, array content Not match the value \n!!!");
			free(arr[n].pt);
			arr[n].pt = NULL;
		}
	}

	free(arr);
	gettimeofday(&end, NULL);
	gap = (end.tv_sec - start.tv_sec)*1000000 + end.tv_usec - start.tv_usec;
	printf("Test done! \n");

	printf(" TOTAL TIME is %d MICRO-SECONDS\n", gap);

	return 0;
}


