#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int ft_popen(const char *file, char *const argv[], char type)
{
	int fd[2];
	pid_t pid;

	if (!file || !argv || (type != 'r' && type != 'w'))
		return (-1);
	
	if (pipe(fd) == -1)
		return (-1);
	
	pid = fork();
	if (pid == -1)
	{
		close(fd[0]);
		close(fd[1]);
		return (-1);
	}

	if (type == 'r')
	{
		if (pid == 0)
		{
			close(fd[0]);
			dup2(fd[1], STDOUT_FILENO);
			close(fd[1]);
			execvp(file, argv);
			exit(-1);
		}
		close(fd[1]);
		return (fd[0]);
	}

	if (type == 'w')
	{
		if (pid == 0)
		{
			close(fd[1]);
			dup2(fd[0], STDIN_FILENO);
			close(fd[0]);
			execvp(file, argv);
			exit(-1);
		}
		close(fd[0]);
		return (fd[1]);
	}
	return (-1);
}
