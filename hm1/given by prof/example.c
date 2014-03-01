#define _XOPEN_SOURCE 500
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Make any use of assert in dbg_print() go away, unless we are actually just testing
// dbg_print().  In that case, define DBG_PRINT_TEST.
#ifndef DBG_PRINT_TEST
#define NDEBUG
#endif
#include <assert.h>

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

    // You should first check to see if there is an appropriate
    // block on a free list, before going to the OS to get memory.

    // This call raises the break by sz bytes and returns a pointer
    // to the current break.
    void *vp = sbrk(sz);

    dbg_print("Allocated block at %lu of %lu bytes.\n", vp, sz);

    return vp;
}

void
free(void *vp) {

    dbg_print("free() called on address %lu.\n", vp);

    // This needs to put the block on the appropriate free list.
}


int
main() {
    
    void *vp = malloc(13);
    free(vp);

    FILE *f1 = fopen("some_file", "w+");
    fclose(f1);

    return 0;
}

