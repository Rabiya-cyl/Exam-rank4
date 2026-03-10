#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int picoshell(char **cmds[])
{
	pid_t	pid;
	int	fd[2];
	int prev_fd;
	int	i;

	prev_fd = -1;
	i = 0;
	if (!cmds || !cmds[0])
		return (1);
	while (cmds[i])
	{
		if (cmds[i + 1] && pipe(fd))
		{
			return (1);
		}
		pid = fork();
		if (pid == -1)
		{
			close(fd[0]);
			close(fd[1]);
			return (1);
		}
		if (pid == 0)
		{
			if (prev_fd != -1)
			{
				if (dup2(prev_fd, STDIN_FILENO) == -1)
				{
					close(prev_fd);
					close(fd[0]);
					close(fd[1]);
					exit(1);
				}
			}
			if (cmds[i + 1] && dup2(fd[1], STDOUT_FILENO) == -1)
			{
				close(fd[0]);
				close(fd[1]);
				exit(1);
			}
			close(fd[0]);
			close(fd[1]);
			execvp(cmds[i][0], cmds[i]);
			exit(0);
		}
		else
		{
			if (prev_fd != -1)
				close(prev_fd);
			if (cmds[i + 1])
			{
				close(fd[1]);
				prev_fd = fd[0];
			}
		}
		i++;
	}
	while (wait(NULL) > 0);
	return (0);
}