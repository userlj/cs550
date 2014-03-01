#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <cstddef>
#include <string>
#include <pthread.h>

using namespace std;



// Thread function.
extern "C" void *handler(void *vp) {

    /*
    Info *info = (Info *) vp;
    int arg1 = info->arg1;
    int arg2 = info->arg2;
    delete info;
    */

    int fd = (unsigned long) vp;
    int ec;

    printf("Handler for %d starting...\n", fd);

    string word;
    enum {LETTER, SPACE} state = SPACE;
    char ch;
    while ((ec = read(fd, &ch, 1)) > 0) {

        printf("Got char %c on conn %d\n", ch, fd);

        switch (state) {
            case LETTER:
                {
                    if (isalnum(ch)) {
                        word.push_back(ch);
                    } else if (isspace(ch)) {
                        printf("---- Word on %d: %s\n", fd, word.c_str());
                        state = SPACE;
                    } else {
                        printf("Bad char on %d.\n", fd);
                        goto exit;
                    }
                }
                break;
            case SPACE:
                {
                    if (isalnum(ch)) {
                        // If it is a letter, we start
                        // accumulating the word.
                        word = ch;
                        state = LETTER;
                    } else if (isspace(ch)) {
                        // We do nothing, since we stay in
                        // the same state.
                    } else {
                        printf("Bad char on %d.\n", fd);
                        goto exit;
                    }
                }
                break;
            default:
                assert(false);
                break;
        }
    }
    exit:

    ec = close(fd); assert(ec == 0);
    printf("Conn %d closed.\n", fd);

    return 0;
}

int main() {

    struct sockaddr_un addr;
    int sock_fd, conn_fd;
    int ec;

    sock_fd = socket(PF_UNIX, SOCK_STREAM, 0); assert(sock_fd >= 0);

    // Intentionally ignore the return value, because this file may not
    // exist.
    unlink("/tmp/server_socket");

    memset(&addr, 0, sizeof addr);

    addr.sun_family = AF_UNIX;
    ec = snprintf(addr.sun_path, sizeof(addr.sun_path), "/tmp/server_socket");
    assert(ec < int(sizeof(addr.sun_path)));

    if (bind(sock_fd, (struct sockaddr *) &addr,
     sizeof addr) != 0) {
        perror("bind");
        exit(1);
    }

    if (listen(sock_fd, 1) != 0) {
        perror("listen");
        exit(1);
    }

    // Make threads start detached, so no need to
    // join.
    pthread_attr_t attr;
    ec = pthread_attr_init(&attr); assert(ec == 0);
    ec = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    assert(ec == 0);

    while (true) {

        socklen_t addr_len = sizeof addr;

        printf("Dispatcher waiting...\n");
        conn_fd = accept(sock_fd, (struct sockaddr *) &addr, &addr_len);
        assert(conn_fd >= 0);
        printf("Dispatcher got conn on %d.\n", conn_fd);

        // Start a worker thread to handle this conn.
        pthread_t tid;
        /*
        Info *info = new Info;
        info->arg1 = 21345;
        info->arg2 = conn_fd;
        */
        ec = pthread_create(&tid, &attr, handler, (void *) uintptr_t(conn_fd)); assert(ec == 0);
    }

    ec = pthread_attr_destroy(&attr); assert(ec == 0);
}