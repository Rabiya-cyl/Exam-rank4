#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

void	handler(int signo, siginfo_t *info, void *context)
{
	(void)signo;
	(void)info;
	(void)context;
}

int sandbox(void (*f)(void), unsigned int timeout, bool verbose)
{
	pid_t	pid;
	struct sigaction act = { 0 };
	int	status;

    act.sa_flags = 0;
    act.sa_sigaction = &handler;
    if (sigaction(SIGALRM, &act, NULL) == -1) {
        perror("sigaction");
        exit(-1);
    }
	pid = fork();
	if (pid == -1)
	{
		perror("fork");
        exit(-1);
	}
	if (pid == 0)
	{
		f();
		exit(0);
	}
	alarm(timeout);
	if (waitpid(pid, &status, 0) == -1)
	{
		if (errno == EINTR) //ATTENTION == pas =!!!!!!!!!
		{
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			if (verbose)
				printf("Bad function: timed out after %d seconds\n", timeout);
			return (0);
		}
		return (-1);
	}
	alarm(0);
	if (WIFEXITED(status))
	{
		if (WEXITSTATUS(status) == 0)
		{
			if (verbose)
				printf("Nice function!\n");
			return (1);
		}
		else
		{
			if (verbose)
				printf("Bad function: exited with code %d\n", WEXITSTATUS(status));
			return (0);
		}
	}
	if (WIFSIGNALED(status))
	{
		int	sig = WTERMSIG(status);
		if (verbose)
			printf("Bad function: %s\n", strsignal(sig));
		return (0);
	}
	return (-1);
}

// TESTS!!!!!!!!!

void	f()
{
	//return ;
	while (1)
	{

	}
}

int main(void)
{
	sandbox(&f, 5, 1);
	return (0);
}
