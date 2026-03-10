#include <unistd.h> // Pour pipe, fork, execvp, dup2, close
#include <stdlib.h> // Pour exit
#include <sys/wait.h> // Pour wait

/*
 * picoshell exécute un pipeline de commandes.
 * cmds: un tableau terminé par NULL de commandes.
 * Chaque commande est elle-même un tableau de chaînes de caractères (style argv)
 * terminé par NULL.
 * Exemple: {{"ls", "-l", NULL}, {"grep", "pico", NULL}, {"wc", "-l", NULL}, NULL}
 */
int picoshell(char **cmds[])
{
    pid_t   pid;       // Pour stocker l'ID du processus retourné par fork()
    int     pipefd[2]; // Tableau pour les descripteurs de fichiers du pipe:
                       // pipefd[0] est pour la lecture, pipefd[1] pour l'écriture.
    int     prev_fd;   // Descripteur de fichier pour la lecture du pipe de la commande *précédente*.
    int     i;         // Compteur pour la boucle sur les commandes.

    // Initialisation. prev_fd est mis à -1 car la première commande n'a pas
    // d'entrée à lire depuis un pipe précédent.
    prev_fd = -1;
    i = 0;

    // Boucle principale : on itère sur chaque commande du tableau `cmds`.
    // La boucle s'arrête quand on atteint le pointeur NULL final.
    while (cmds[i])
    {
        // --- 1. Création du pipe ---
        // S'il y a une commande suivante, on doit créer un pipe pour la connecter à la commande actuelle.
        // La sortie de `cmds[i]` ira dans l'entrée de `cmds[i+1]`.
        if (cmds[i + 1] && pipe(pipefd))
            return (1); // Si pipe() échoue, on retourne une erreur.

        // --- 2. Création du processus enfant ---
        pid = fork();
        if (pid == -1) // Si fork() échoue.
        {
            // Si on avait créé un pipe, il faut le fermer avant de quitter.
            if (cmds[i + 1])
            {
                close(pipefd[0]);
                close(pipefd[1]);
            }
            return (1);
        }

        // --- 3. Code du processus enfant ---
        if (pid == 0)
        {
            // Redirection de l'entrée standard (stdin)
            // Si ce n'est pas la première commande (`prev_fd` est valide),
            // on redirige son entrée standard pour lire depuis le pipe précédent.
            if (prev_fd != -1)
            {
                // dup2 copie le descripteur prev_fd sur STDIN_FILENO (0).
                if (dup2(prev_fd, STDIN_FILENO) == -1)
                    exit(1); // Si dup2 échoue, l'enfant quitte.
                close(prev_fd); // On ferme le descripteur original, devenu inutile.
            }

            // Redirection de la sortie standard (stdout)
            // Si ce n'est pas la dernière commande, on redirige sa sortie standard
            // pour écrire dans le pipe qu'on a créé.
            if (cmds[i + 1])
            {
                // L'enfant n'a besoin que d'écrire dans le pipe, pas de lire. On ferme donc la lecture.
                close(pipefd[0]);
                // On redirige stdout (1) vers l'extrémité d'écriture du pipe.
                if (dup2(pipefd[1], STDOUT_FILENO) == -1)
                    exit(1);
                close(pipefd[1]); // On ferme le descripteur original.
            }

            // Exécution de la commande. `execvp` remplace le code de l'enfant
            // par le code de la commande à exécuter.
            execvp(cmds[i][0], cmds[i]);
            // Si execvp réussit, le code suivant n'est jamais atteint.
            // Si execvp échoue (ex: commande non trouvée), l'enfant doit se terminer.
            exit(1);
        }

        // --- 4. Code du processus parent ---
        // Le parent doit fermer les descripteurs de fichiers qu'il n'utilisera pas.

        // Ferme la lecture du pipe *précédent*. Le parent n'en a plus besoin.
        if (prev_fd != -1)
            close(prev_fd);

        // Si on a créé un pipe pour la commande suivante...
        if (cmds[i + 1])
        {
            // Le parent doit fermer l'extrémité d'écriture du pipe. C'est CRUCIAL.
            // S'il ne le fait pas, le prochain enfant qui lira sur pipefd[0]
            // ne recevra jamais de signal de fin de fichier (EOF) et restera bloqué.
            close(pipefd[1]);
            // On sauvegarde l'extrémité de lecture du pipe actuel.
            // Elle deviendra l'entrée (prev_fd) pour la prochaine itération de la boucle.
            prev_fd = pipefd[0];
        }

        // On passe à la commande suivante.
        i++;
    }

    // --- 5. Attente des enfants ---
    // Le parent attend que tous les processus enfants se terminent.
    // wait(NULL) attend un enfant. La boucle continue tant qu'il y a des enfants à attendre.
    while (wait(NULL) > 0);

    // Toutes les commandes ont été exécutées avec succès.
    return (0);
}