#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <string>

using namespace std;



int main() {

    struct sockaddr_un addr;
    int sock_fd, conn_fd;
    int ec;

    sock_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    assert(sock_fd >= 0);

    // Intentionally ignore the return value, because this
    // file may not exist.
    unlink("/tmp/server_socket");

    memset(&addr, 0, sizeof addr);

    // Put path in addr.
    addr.sun_family = AF_UNIX;
    ec = snprintf(addr.sun_path, sizeof(addr.sun_path),
     "/tmp/server_socket");
    assert(ec < int(sizeof(addr.sun_path)));

    // Bind connection addr to sock.
    if (bind(sock_fd, (struct sockaddr *) &addr,
     sizeof addr) != 0) {
        perror("bind");
        exit(1);
    }

    // Set queue size for pending connections.
    if (listen(sock_fd, 1) != 0) {
        perror("listen");
        exit(1);
    }

    while (true) {

        socklen_t addr_len = sizeof addr;

        printf("Waiting for conn..."); fflush(stdout);
        // Wait for connections.
        conn_fd = accept(sock_fd,
         (struct sockaddr *) &addr, &addr_len);
        assert(conn_fd >= 0);
        printf("got on file descr %d.\n", conn_fd);

        // Loop, reading from connection.
        string word; // Build word in this str.
        enum {LETTER, SPACE} state = SPACE;
        char ch;
        while ((ec = read(conn_fd, &ch, 1)) > 0) {

            printf("Got char %c on conn %d\n", ch, conn_fd);

            switch (state) {
                case LETTER:
                    {
                        if (isalnum(ch)) {
                            word.push_back(ch);
                        } else if (isspace(ch)) {
                            printf("---- Word on %d: %s\n",
                             conn_fd, word.c_str());
                            state = SPACE;
                        } else {
                            printf("Bad char on %d.\n",
                             conn_fd);
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
                            // We do nothing, since we stay
                            // in the same state.
                        } else {
                            printf("Bad char on %d.\n", conn_fd);
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
        assert(ec == 0);
        printf("Conn %d closed.\n", conn_fd);

        ec = close(conn_fd);
        assert(ec == 0);
    }
}