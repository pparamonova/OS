#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>

static void int_to_str(int val, char *buf) {
    if (val == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    int i = 0;
    char tmp[16];
    while (val > 0) {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    }
    for (int j = 0; j < i; j++) {
        buf[j] = tmp[i - 1 - j];
    }
    buf[i] = '\0';
}

int main(void) {
    int pipe1[2], pipe2[2];

    if (pipe(pipe1) < 0 || pipe(pipe2) < 0) {
        const char *err = "pipe error\n";
        write(2, err, strlen(err));
        return 1;
    }

    const char *prompt = "Enter filename: ";
    write(STDOUT_FILENO, prompt, strlen(prompt));

    char filename[256];
    int flen = 0;
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != '\n' && flen < 255) {
        filename[flen++] = c;
    }
    filename[flen] = '\0';

    pid_t pid = fork();
    if (pid < 0) {
        const char *err = "fork error\n";
        write(2, err, strlen(err));
        return 1;
    }

    if (pid == 0) {
        close(pipe1[1]);
        close(pipe2[0]);

        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            const char *err = "open error\n";
            write(2, err, strlen(err));
            _exit(1);
        }

        dup2(pipe1[0], STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        close(pipe1[0]);
        close(fd);

        char fd_str[16];
        int_to_str(pipe2[1], fd_str);

        execl("./child", "child", fd_str, (char *)NULL);
        const char *err = "execl error\n";
        write(2, err, strlen(err));
        _exit(1);
    }

    close(pipe1[0]);
    close(pipe2[1]);

    const char *info = "Enter strings (Ctrl+D to finish):\n";
    write(STDOUT_FILENO, info, strlen(info));

    char in_buf[4096];
    char err_buf[4096];
    int child_alive = 1;

    while (child_alive) {
        fd_set rfds;
        FD_ZERO(&rfds);

        int maxfd = pipe2[0];
        FD_SET(pipe2[0], &rfds);

        if (child_alive) {
            FD_SET(STDIN_FILENO, &rfds);
            if (STDIN_FILENO > maxfd) maxfd = STDIN_FILENO;
        }

        int ret = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (ret < 0) break;

        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            ssize_t n = read(STDIN_FILENO, in_buf, sizeof(in_buf) - 1);
            if (n <= 0) {
                close(pipe1[1]);
                child_alive = 0;
            } else {
                write(pipe1[1], in_buf, n);
            }
        }

        if (FD_ISSET(pipe2[0], &rfds)) {
            ssize_t n = read(pipe2[0], err_buf, sizeof(err_buf) - 1);
            if (n > 0) {
                write(STDOUT_FILENO, err_buf, n);
            }
        }
    }

    ssize_t n;
    while ((n = read(pipe2[0], err_buf, sizeof(err_buf) - 1)) > 0) {
        write(STDOUT_FILENO, err_buf, n);
    }

    close(pipe1[1]);
    close(pipe2[0]);

    int status;
    waitpid(pid, &status, 0);

    const char *done = "\nDone.\n";
    write(STDOUT_FILENO, done, strlen(done));
    return 0;
}