// Define this to pick up the prototype for sbrk().
#define _XOPEN_SOURCE 500
#include "malloc.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

// Make any use of assert in dbg_print() go away, unless we are actually just testing
// dbg_print().  In that case, define DBG_PRINT_TEST.
#ifndef DBG_PRINT_TEST
#define NDEBUG
#endif
#include <assert.h>

//#define DEBUG
#undef  DEBUG
#define BASE    8
#define RANGE   32
void *freelist[RANGE] = { NULL };
void *uselist[RANGE] = { NULL };

#define ALIGN(x, a) ((x)&(~((a) - 1)))
struct Node {
    unsigned int index;
    void *alignaddr;
    void *sysaddr;
    struct Node *next;
};

void
dbg_print(const char *fmt, ...);


void traverse(struct Node **head) {
    struct Node *tmp;
    int i = 0;
    for (i = 0; i < 31; i++) {
        if (head[i]) {
            tmp = head[i];
            while (tmp) {
                dbg_print("%lu: %lu: %lu\n", i, tmp->index, tmp->alignaddr);
                tmp = tmp->next;
            }
        }
    }
}

size_t getindex(size_t size)
{
    size_t i = 0;
    size_t power = 1;

    while(power < size)
    {
        power = power *2;
        i++;
    }

    return i;

}

struct Node *get_available(struct Node **freelist, int index)
{
    struct Node *head = freelist[index];
    struct Node *avil = NULL;
#ifdef  DEBUG
    dbg_print("get_available called with: %lu\n", index);
    dbg_print("***** freelist *****\n");
    traverse(freelist);
    dbg_print("***** end freelist *****\n");
#endif
    if (head) {
        avil = head;
        freelist[index] = head->next;
    }

    if (avil) {
        avil->next = uselist[index];
        uselist[index] = avil;
    }

    return avil;
}

struct Node *find_node(void *ptr, void **pprev)
{
    int i;
    struct Node *cur;
    for (i = 0; i < 32; i++) {
        if (uselist[i]) {
            cur = uselist[i];
            while (cur && (unsigned long)(cur->alignaddr) != (unsigned long)ptr) {
                if (pprev)
                    *(struct Node**)pprev = cur;
                cur = cur->next;
            }

            if (cur) {
#ifdef  DEBUG
                dbg_print("find_node index: %lu : %lu\n", cur->index,
                        cur->alignaddr);
                dbg_print("----- uselist -----\n");
                traverse(uselist);
                dbg_print("----- end uselist -----\n");
#endif
                return cur;
            }
        }
    }
    return NULL;
}

static void
error() {
    const char *const m = "\ndbg_print(): Bad format.\n";
    write(2, m, strlen(m));
}

static char *
ultoa_helper(char *buf, unsigned long ul) {
    if (ul > 0) {
        buf = ultoa_helper(buf, ul/10);
        *buf++ = '0' + ul%10;
    }
    return buf;
}

// Convert ulong to ASCII.
static void
ultoa(char *buf, unsigned long ul) {
    assert(buf != 0);
    if (ul == 0) {
        *buf++ = '0';
    } else {
        buf = ultoa_helper(buf, ul);
    }
    *buf = '\0';
}

// Push diag context so we can ignore unused vars.
#pragma GCC diagnostic push

#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

// This is a tiny version of printf() that accepts only %lu and %%.
void
dbg_print(const char *fmt, ...) {

    enum { ST_CHUNK = 1, ST_PERCENT, ST_L } state = ST_CHUNK;
    size_t chunk_begin = 0;
    int ec;

    va_list ap;
    va_start(ap, fmt);

    size_t i = 0;
    for (; fmt[i] != '\0'; i++) {
        int ch = fmt[i];
        switch (state) {
            case ST_CHUNK:
                {
                    if (ch == '%') {
                        state = ST_PERCENT;
                        ec = write(2, &fmt[chunk_begin], i - chunk_begin);
                        assert((unsigned) ec == i - chunk_begin);
                    }
                }
                break;
            case ST_PERCENT:
                {
                    if (ch == 'l') {
                        state = ST_L;
                    } else if (ch == '%') {
                        chunk_begin = i;
                        state = ST_CHUNK;
                    } else {
                        error();
                    }
                }
                break;
            case ST_L:
                {
                    if (ch == 'u') {
                        unsigned long ul = va_arg(ap, unsigned long);
                        char buf[100];
                        ultoa(buf, ul);
                        ec = write(2, buf, strlen(buf));
                        assert((unsigned) ec == strlen(buf));
                        chunk_begin = i + 1;
                        state = ST_CHUNK;
                    } else {
                        error();
                    }
                }
                break;
            default:
                assert(0);
                abort();
                break;
        }
    }

    if (i - chunk_begin > 0) {
        ec = write(2, &fmt[chunk_begin], i - chunk_begin);
        assert((unsigned) ec == i - chunk_begin);
    }

    va_end(ap);
}

#pragma GCC diagnostic pop

void *
malloc(size_t sz) {
    size_t  size = 0;
    struct Node *node;
    void *rp = NULL;
    void *vp = NULL;

    if(0 == sz)
        return NULL;
    // You should first check to see if there is an appropriate
    // block on a free list, before going to the OS to get memory.

    size_t index = getindex(sz);
    node = get_available((struct Node**)freelist, index);
    if (node) {
        vp = node->alignaddr;
        goto found;
    }

    size = (1 << index) + sizeof(struct Node) + BASE - 1;
    rp = sbrk(size);
    if (rp == (void*)-1) {
        dbg_print("sbrk malloc error\n");
        exit(1);
    }
    node = (struct Node*)rp;
    node->sysaddr = node + 1;
    node->alignaddr = vp = 
        (void *)ALIGN((unsigned long)(node->sysaddr) + BASE -1, BASE);
    node->index = index;
    node->next = uselist[index];
    uselist[index] = node;

#ifdef DEBUG
    dbg_print("----- uselist -----\n");
    traverse(uselist);
    dbg_print("----- end uselist -----\n");
#endif

found: 
    //dbg_print("Allocated block at %lu : %lu of %lu bytes.\n\n", index, vp, sz);
    return vp;
}

void
free(void *vp) {
    struct Node *cur, *prev;
    //dbg_print("\nfree() called on address %lu.\n", vp);
    if (!vp)
        return;
    cur = find_node(vp, (void**)&prev);
    if (cur) {
        if (cur == uselist[cur->index]) {
            uselist[cur->index] = cur->next;
        } else {
            prev->next = cur->next;
            //dbg_print("free next is %lu\n", (unsigned long)prev->next);
        }
        cur->next = freelist[cur->index];
        freelist[cur->index] = cur;
#ifdef  DEBUG
        dbg_print("free success in %lu\n\n", cur->index);
        dbg_print("----- uselist -----\n");
        traverse(uselist);
        dbg_print("----- end uselist -----\n");
#endif
        return;
    }

    dbg_print("free error\n\n");
}

void *
realloc(void *ptr, size_t size)
{
    void *newptr;
    struct Node *node;
    struct Node *new;
    size_t  cur_size;
    unsigned int i, new_index;

    //dbg_print("\nReallocated block called here: %lu \n", ptr);
    if (!ptr)
        return NULL;

    if (!size) {
        free(ptr);
        return NULL;
    }

    node = find_node(ptr, NULL);
    if (!node)
        return NULL;

    newptr = malloc(size);
    new = find_node(newptr, NULL);
    new_index = 1 >> new->index;
    cur_size = node->index > new->index ? new_index : size;
    for (i = 0; i < cur_size; i++)
        *(char*)new->alignaddr = *(char*)node->alignaddr;

    free(ptr);
    return newptr;
}

void *
calloc(size_t num, size_t size)
{
    void *ptr;
    size_t  totalsize;

#ifdef DEBUG
    dbg_print("\ncalloc block called here \n");
    dbg_print("num : %lu, size: %lu\n", num, size);
#endif

    if (!num || !size)
        return NULL;

    totalsize = num * size;
    ptr = malloc(totalsize);
    return ptr;
}

