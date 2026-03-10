#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h> // Nécessaire pour exit()

int ft_popen(const char *file, char *const argv[], char type)
{
    int     fd[2];
    pid_t   pid;

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
    if (pid == 0) // Processus enfant
    {
        if (type == 'r')
        {
            // L'enfant doit écrire dans le pipe, donc on connecte sa sortie standard
            // à l'extrémité d'écriture du pipe (fd[1]).
            close(fd[0]); // L'enfant ne lit pas depuis le pipe.
            if (dup2(fd[1], STDOUT_FILENO) == -1)
            {
                close(fd[1]);
                exit(1);
            }
            close(fd[1]);
        }
        else // type == 'w'
        {
            // L'enfant doit lire depuis le pipe, donc on connecte son entrée standard
            // à l'extrémité de lecture du pipe (fd[0]).
            close(fd[1]); // L'enfant n'écrit pas dans le pipe.
            if (dup2(fd[0], STDIN_FILENO) == -1)
            {
                close(fd[0]);
                exit(1);
            }
            close(fd[0]);
        }
        execvp(file, argv);
        // Si execvp échoue, le processus enfant se termine.
        exit(1);
    }
    // Processus parent
    if (type == 'r')
    {
        // Le parent veut lire la sortie de l'enfant.
        // CORRECTION : On ferme l'extrémité d'écriture, car le parent ne fait que lire.
        close(fd[1]);
        // CORRECTION : On retourne l'extrémité de lecture.
        return (fd[0]);
    }
    else // type == 'w'
    {
        // Le parent veut écrire vers l'entrée de l'enfant.
        // CORRECTION : On ferme l'extrémité de lecture, car le parent ne fait qu'écrire.
        close(fd[0]);
        // CORRECTION : On retourne l'extrémité d'écriture.
        return (fd[1]);
    }
}