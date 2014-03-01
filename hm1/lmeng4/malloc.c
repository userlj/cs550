#define _XOPEN_SOURCE 500
#define ALLOC_LIST_SIZE 100
#define FREE_LIST_SIZE 32


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


size_t check_size(size_t size);
unsigned long power(int root, int k);
int log_2(size_t size);


void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);


// memory block
typedef struct memBlk{  
	void *addr;
	size_t size;
	struct memBlk *next;
}memBlk;


// memory blocks that have been allocated
memBlk *alloc_list[ALLOC_LIST_SIZE] = {NULL};

// memory blocks that have been freed
memBlk *free_list[FREE_LIST_SIZE] = {NULL}; 


// check if the size of memory block is 2 to the power of n
size_t check_size(size_t size) 
{
        int i;
        if(size==1) return 1;
        for(i=0; i<32; i++)
        {
                if(size>power(2,i) && size<=power(2,i+1))
                {
                        size = power(2,i+1);
                        break;
                }
        }
	return size;
}


// calculate the index of free_list from a given address
int log_2(size_t size)
{
	int i = 0;
	size_t n = 1;
	while(n!=size)
	{
		n *= 2;
		i ++;
	}
	return i;
}


// calculate root to the power of k
unsigned long power(int root, int k)
{
        int i = 0;
        int result = 1;
        for(i=0;i<k;i++)
        {
                result *= root;
        }
        return (unsigned long) result;
}


// allocates a block of memory of size bytes
void *malloc(size_t size)
{
	if(size==0) return NULL;
	size = check_size(size);  // make size to 2^n
	int n = log_2(size);  // index of free_list
	if(free_list[n]!=NULL)
	{
		void *new_addr = free_list[n]->addr;
		free_list[n] = free_list[n]->next;   // here
		return new_addr;
	}	
	else
	{
		struct memBlk *new_memblk = (struct memBlk *) sbrk(sizeof(struct memBlk));
		// current address
		void *new_addr = sbrk(0);
		// see if the address is multiple of 8
		int diff = 8 - (((unsigned long)new_addr)&7);   // here
		// make the address to multiple of 8
		sbrk(diff);
		new_addr = sbrk(size);
		// get the index for alloc_list
		int alloc_list_index = (((unsigned long)new_addr)%ALLOC_LIST_SIZE);  // here
		new_memblk->addr = new_addr;
		new_memblk->size = size;
		new_memblk->next = NULL;

		new_memblk->next = alloc_list[alloc_list_index];
		alloc_list[alloc_list_index] = new_memblk;
		return new_addr;
	}
}


//frees a block of memory that had previously been allocated
void free(void *ptr)
{
	if(ptr==NULL) return;
	unsigned long ul_ptr = (unsigned long)ptr;

	int alloc_list_index = ul_ptr%ALLOC_LIST_SIZE;
	struct memBlk *tmp = alloc_list[alloc_list_index];
	while(tmp->addr!=ptr)
	{
		tmp = tmp->next;
	}
	int free_list_index = log_2(tmp->size);
        struct memBlk *new_memblk = (struct memBlk *) sbrk(sizeof(struct memBlk));
        new_memblk->addr = tmp->addr;
        new_memblk->size = tmp->size;
        new_memblk->next = free_list[free_list_index];
	free_list[free_list_index] = new_memblk;
}


//allocates memory for an array of nmemb elements of size bytes each 
//and returns a pointer to the allocated memory
void *calloc(size_t nmemb, size_t size)
{
	if(nmemb==0 || size==0) return NULL;
	void *new_addr = malloc(nmemb*size);
	memset(new_addr,0,nmemb*size);
	return new_addr;
}


//changes the size of the memory block pointed to by ptr to size bytes
void *realloc(void *ptr, size_t size)
{
	void *new_addr = NULL;
	if(ptr==NULL)
	{
		new_addr = malloc(size);
		return new_addr;
	}
	else
	{
		if(size==0)
		{
			free(ptr);
			return NULL;
		}
		else
		{
			unsigned long ul_ptr = (unsigned long) ptr;
	        	int alloc_list_index = ul_ptr%ALLOC_LIST_SIZE;
        		struct memBlk *tmp = alloc_list[alloc_list_index];  
        		while(tmp->addr!=ptr)
        		{
        	        	tmp = tmp->next;
       			}

			if(size<=(tmp->size)) return tmp->addr;   // here
			else
			{
				new_addr = malloc(size);
				memmove(new_addr,tmp->addr,tmp->size);
				free(ptr);
				return new_addr;		
			}
		}
	}
}
