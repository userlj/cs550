#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>



// Saved terminal state.
struct termios save_termios;
// Restore the terminal state.
void tty_reset() {
    int ec = tcsetattr(0, TCSAFLUSH, &save_termios);
    if (ec < 0) {
        perror("tcsetattr");
        assert(false);
    }
}

void tty_cbreak() {

    termios buf;
    int ec;

    // Get current state.
    ec = tcgetattr(0, &buf); assert(ec == 0);
    save_termios = buf;

    // Echo off, canonical mode off.
    buf.c_lflag &= ~(ECHO | ICANON);

    // One byte at a time, no timer.
    buf.c_cc[VMIN] = 1;
    buf.c_cc[VTIME] = 0;

    // Set new state.
    ec = tcsetattr(0, TCSAFLUSH, &buf); assert(ec == 0);

    // Verify that the changes stuck.  tcsetattr
    // can return 0 on partial success.
    ec = tcgetattr(0, &buf); assert(ec == 0);
    if ((buf.c_lflag & (ECHO | ICANON))
     || buf.c_cc[VMIN] != 1 || buf.c_cc[VTIME] != 0) {
        // Only some of the changes were made.  Restore the
        // original settings.
        assert(false);
    }
}

// Signal handler to restore terminal state.
extern "C" void int_handler(int sig) {
    tty_reset();
    exit(1);
}

int main(void) {

    struct sockaddr_un addr;
    int  conn_fd;
    int ec;

    conn_fd = socket(PF_UNIX, SOCK_STREAM, 0);
    assert(conn_fd >= 0);

    memset(&addr, 0, sizeof addr);

    addr.sun_family = AF_UNIX;
    ec = snprintf(addr.sun_path, sizeof(addr.sun_path),
     "/tmp/server_socket");
    assert(ec < int(sizeof(addr.sun_path)));

    printf("Connecting..."); fflush(stdout);
    if (connect(conn_fd, (struct sockaddr *) &addr,
     sizeof addr) != 0) {
        perror("connect");
        exit(1);
    }
    printf("connected.\n");

    // Turn off line processing in term driver.
    tty_cbreak();

    // Ignore SIGPIPE.
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    ec = sigaction(SIGPIPE, &sa, 0); assert(ec == 0);

    // Reset and exit on SIGINT.
    sa.sa_handler = int_handler;
    ec = sigaction(SIGINT, &sa, 0); assert(ec == 0);

    // Loop, reading one char at a time and sending it to
    // the server.
    char ch;
    int n;
    while ((n = read(0, &ch, 1)) == 1) {

        // printf("Got: %c, %u\n", ch, (unsigned int) ch);

        // Handle ^D, used to close input.
        if (ch == 4) {
            break;
        }

        // Write to server.
        ec = write(conn_fd, &ch, 1);
        if (ec < 1) {
            break;
        }

        // Echo to stdout.
        ec = write(1, &ch, 1); assert(ec == 1);
    }

    ec = close(conn_fd); assert(ec == 0);
    tty_reset();
}