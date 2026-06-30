#include <unistd.h>     // pipe, fork, dup2, execvp, close
#include <stdlib.h>     // exit
#include <sys/types.h>  // pid_t

int ft_popen(const char *file, char *const av[], char type)
{
    int fd[2];
    pid_t pid;
    
    if (!file || !av || (type != 'r' && type != 'w') || pipe(fd) < 0)
        return -1;
    if ((pid = fork()) < 0)
    {
        close(fd[0]);
        close(fd[1]);
        return -1;
    }
    if (pid == 0)
    {
        // close(fd[type == 'r' ? 0 : 1]); // to avoid double close
        dup2(fd[type == 'r' ? 1 : 0], type == 'r' ? 1 : 0);
        close(fd[0]);
        close(fd[1]);
        execvp(file, av);
        exit(1);
    }
    close(fd[type == 'r' ? 1 : 0]);
    return fd[type == 'r' ? 0 : 1];
}