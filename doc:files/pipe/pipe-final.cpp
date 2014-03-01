#include <stdio.h>
#include <unistd.h>
#include <assert.h>

int main() {
    
    int rv;
    int fds[2];

    // fds[0] is read end.
    rv = pipe(fds); assert(rv == 0);

    rv = fork();
    // Child gets returned 0.
    if (rv == 0) {
        rv = dup2(fds[0], 0); assert(rv == 0);
        rv = close(fds[0]); assert(rv == 0);
        rv = close(fds[1]); assert(rv == 0);
        int ch;
        while ((ch = getchar()) != EOF) {
            printf("Child received: %c\n", char(ch));
        }
        printf("Child received EOF.\n");
    } else {
        assert(rv > 0);
        rv = dup2(fds[1], 1); assert(rv == 1);
        rv = close(fds[0]); assert(rv == 0);
        rv = close(fds[1]); assert(rv == 0);
        setbuf(stdout, nullptr);
        int ch;
        while ((ch = getchar()) != EOF) {
            putchar(ch);
        }
        fprintf(stderr, "Parent received EOF.\n");
    }
}