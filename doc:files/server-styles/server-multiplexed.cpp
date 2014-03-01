#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <string>
#include <utility>
#include <map>
#include <poll.h>

using namespace std;

enum State {LETTER, SPACE};
struct Context {
    Context(int fd_) : fd(fd_), state(SPACE) {}
    const int fd;
    State state;
    string word;
};


int main() {

    struct sockaddr_un addr;
    int sock_fd;
    int ec;

    sock_fd = socket(PF_UNIX, SOCK_STREAM, 0); assert(sock_fd >= 0);

    // Intentionally ignore the return value, because this
    // file may not exist.
    unlink("/tmp/server_socket");

    memset(&addr, 0, sizeof addr);

    addr.sun_family = AF_UNIX;
    ec = snprintf(addr.sun_path, sizeof(addr.sun_path),
     "/tmp/server_socket");
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

    // Holds contexts, and maps conn fd to context.
    typedef map<int, Context> conn_table_t;
    conn_table_t conn_table;
    // Used for poll().
    pollfd *fds_array = 0;
    // Used to flag that the above array needs to be
    // rebuilt, based on contents of conn_table.
    bool rebuild = true;

    while (true) {

        if (rebuild) {
            delete [] fds_array;
            fds_array = new pollfd[conn_table.size() + 1];
            fds_array[0].fd = sock_fd;
            fds_array[0].events = POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI;
            int i = 1;
            for (auto it : conn_table) {
                fds_array[i].fd = it.second.fd;
                fds_array[i].events
                 = POLLIN|POLLRDNORM|POLLRDBAND|POLLPRI;
                i++;
            }
            rebuild = false;
        }
        // -1 timeout means wait forever.
        ec = poll(fds_array, conn_table.size() + 1, -1);
        if (ec < 0) {
            perror("poll");
            exit(1);
        } else if (ec == 0) {
            // This means timeout, which should not happen.
            assert(false);
        }

        for (size_t i = 0; i < conn_table.size() + 1; i++) {

            pollfd &pfd(fds_array[i]);

            // The first fd is always the listen socket.
            if (i == 0) {

                assert(pfd.fd == sock_fd);

                // This means that a connection is ready.
                if (pfd.revents&POLLIN) {

                    socklen_t addr_len = sizeof addr;

                    printf("Waiting for conn...");
                    fflush(stdout);
                    int conn_fd = accept(sock_fd,
                     (struct sockaddr *) &addr, &addr_len); assert(conn_fd >= 0);
                    printf("got on file descr %d.\n", conn_fd);

                    auto res = conn_table.insert(make_pair(conn_fd, Context(conn_fd)));
                    assert(res.second);
                    // We have a new connection, so the
                    // fds_array needs to be rebuilt.
                    rebuild = true;

                } else {

                    assert(pfd.revents == 0);
                }

            } else {

                const int fd = pfd.fd;

                // Something to be read.
                if (pfd.revents&POLLIN) {

                    auto it = conn_table.find(pfd.fd);
                    assert(it != conn_table.end());
                    Context &ctx(it->second);

                    string &word(ctx.word);
                    enum State &state(ctx.state);
                    char ch;

                    struct E {};

                    try {

                        ec = read(fd, &ch, 1);
                        if (ec != 1) { throw E(); }

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
                                        throw E();
                                    }
                                }
                                break;
                            case SPACE:
                                {
                                    if (isalnum(ch)) {
                                        // If it is a letter, we start accumulating the
                                        // word.
                                        word = ch;
                                        state = LETTER;
                                    } else if (isspace(ch)) {
                                        // We do nothing, since we stay in the same state.
                                    } else {
                                        printf("Bad char on %d.\n", fd);
                                        throw E();
                                    }
                                }
                                break;
                            default:
                                assert(false);
                                break;
                        }

                    } catch (const E &) {

                        assert(ec == 0 || ec == 1);

                        if (state == LETTER) {
                            printf("---- Word on %d: %s\n", fd, word.c_str());
                        }
                            
                        ec = close(fd); assert(ec == 0);
                        printf("Conn %d closed.\n", fd);
                        conn_table.erase(it);
                        // This connection needs to be
                        // removed from fds_array, so
                        // rebuild.
                        rebuild = true;
                    }

                } else {

                    assert(pfd.revents == 0);
                }
            }
        }
    }
}