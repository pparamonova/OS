#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        const char *msg = "Usage: child <err_fd>\n";
        write(2, msg, strlen(msg));
        return 1;
    }

    int err_fd = atoi(argv[1]);
    char buf[4096];
    int pos = 0;
    char c;

    while (read(STDIN_FILENO, &c, 1) == 1)
    {
        if (c == '\n' || pos >= (int)sizeof(buf) - 1)
        {
            buf[pos] = '\0';

            if (pos > 0)
            {
                if (buf[0] >= 'A' && buf[0] <= 'Z')
                {
                    write(STDOUT_FILENO, buf, pos);
                    const char nl = '\n';
                    write(STDOUT_FILENO, &nl, 1);
                }
                else
                {
                    const char *prefix = "Error: \"";
                    const char *suffix = "\" does not start with uppercase\n";

                    write(err_fd, prefix, strlen(prefix));
                    write(err_fd, buf, pos);
                    write(err_fd, suffix, strlen(suffix));
                }
            }
            pos = 0;
        }
        else
        {
            buf[pos++] = c;
        }
    }

    close(err_fd);
    return 0;
}