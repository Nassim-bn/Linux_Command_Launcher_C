#include "fonctions_utilitaires.h"

#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "file_synchronisee.h"
// Pour la file
void P(sem_t *semaphore) {
    if (sem_wait(semaphore) == -1) {
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
}

void V(sem_t *semaphore) {
    if (sem_post(semaphore) == -1) {
        perror("sem_post");
        exit(EXIT_FAILURE);
    }
}

// Pour le demon
void extraire_commandes(char *chaine_de_characteres, char ***commandes, int *nombre_commandes) {
    char *token;
    const char *delimiteur = "|";
    *nombre_commandes = 0;

    token = strtok(chaine_de_characteres, delimiteur);
    *commandes = (char **)malloc(sizeof(char *));

    while (token != NULL) {
        *commandes = (char **)realloc(*commandes, sizeof(char *) * ((size_t)*nombre_commandes + 1));
        (*commandes)[*nombre_commandes] = strdup(token);
        (*nombre_commandes)++;
        token = strtok(NULL, delimiteur);
    }
}

void *routine_thread(void *arg) {
    pid_t pid_client = *(pid_t *)arg;
    char buffer[TAILLE];

    char *tube_entree = malloc(sizeof(char) * 20);
    snprintf(tube_entree, 20, "tube_%d_entree", pid_client);

    char *tube_sortie = malloc(sizeof(char) * 20);
    snprintf(tube_sortie, 20, "tube_%d_sortie", pid_client);

    char *tube_erreur = malloc(sizeof(char) * 20);
    snprintf(tube_erreur, 20, "tube_%d_erreur", pid_client);

    free(arg);

    // ouverture du tube en lecture
    int fd_entree = open(tube_entree, O_RDONLY);
    if (fd_entree == -1) {
        perror("open, tube_entree");
        exit(EXIT_FAILURE);
    }
    // lecture des commande du client
    int n = (int)read(fd_entree, buffer, sizeof(TAILLE));
    if (n < 0) {
        perror("read");
        exit(EXIT_FAILURE);
    }
    printf("    - Lecture depuis le tube d'entrée effectuée avec succès\n");

    // Extraction des commandes
    char **commandes;
    int nombre_commandes;
    extraire_commandes(buffer, &commandes, &nombre_commandes);
    char *cmd;
    printf("    - Extraction de commandes effectuée avec succès\n");
    // Exécution des commandes
    int fd_in = -1;

    for (int i = 0; i < nombre_commandes - 1; i++) {
        // du premier processus à l'avant dernier
        // création du tube
        int tube[2];

        if (pipe(tube) == -1) {
            perror("pipe, création");
            exit(EXIT_FAILURE);
        }
        // Création de plusieurs processus fils, chaque processus va exécuter une seule commande
        switch (fork()) {
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
                break;
            case 0:
                if (fd_in != -1) {
                    if (dup2(fd_in, STDIN_FILENO) == -1) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                    if (close(fd_in)) {
                        perror("close");
                        exit(EXIT_FAILURE);
                    }
                }
                // redirection de la sortie standard
                if (dup2(tube[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                for (int j = 0; j < 2; j++) {
                    if (close(tube[j]) == -1) {
                        perror("close");
                        exit(EXIT_FAILURE);
                    }
                }
                // Exécuter la commande
                cmd = commandes[i];
                execvp(cmd, &cmd);
                perror("execvp");
                exit(EXIT_FAILURE);
            default:
                wait(NULL);
                // fermer le fd en écriture du tube dans le processus père
                if (close(tube[1]) == -1) {
                    perror("close");
                    exit(EXIT_FAILURE);
                }
                // retourner fd en lecture de lecture du tube pour la prochaine commande
                fd_in = tube[0];
                break;
        }

        if (fd_in == -1) {
            fprintf(stderr, "Erreur lors de l'exécution de la commande %s\n", commandes[i]);
            exit(EXIT_FAILURE);
        }
    }
    printf("    - Exécution de commandes effectuée avec succès\n");

    // le résultat du dernier processus sera retourné au client à travers le tube adéquat

    cmd = commandes[nombre_commandes - 1];  // la dernière commande
    // Ouverture fd en ecriture pour répondre au client
    int fd_out = open(tube_sortie, O_WRONLY);
    if (fd_out == -1) {
        perror("open, tube_sortie");
        exit(EXIT_FAILURE);
    }

    // Ouverture du tube d'erreur
    int fd_erreur = open(tube_erreur, O_WRONLY);
    if (fd_erreur == -1) {
        perror("open, tube_erreur");
        exit(EXIT_FAILURE);
    }

    switch (fork()) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
            break;
        case 0:
            if (fd_in != -1) {
                // Redirection de l'entrée standard
                if (dup2(fd_in, STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                if (close(fd_in) == -1) {
                    perror("Close, fd_in");
                    exit(EXIT_FAILURE);
                }
            }
            // Redirection de la sortie standard vers le client

            if (dup2(fd_out, STDOUT_FILENO) == -1) {
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            if (close(fd_out) == -1) {
                perror("close_fd_out");
                exit(EXIT_FAILURE);
            }

            // Exécuter la commande
            execvp(cmd, &cmd);

            // Cette partie du code n'est exécutée que lorsqu'il y ait un erreur

            char message_erreur[2048];

            snprintf(message_erreur, 2048, "Erreur lors de l'exécution de la commande %s\n", cmd);
            if (write(fd_erreur, message_erreur, strlen(message_erreur)) <= 0) {
                perror("write");
                exit(EXIT_FAILURE);
            }
            if (close(fd_erreur) == -1) {
                perror("close, fd_erreur");
                exit(EXIT_FAILURE);
            }

            perror("execvp");
            exit(EXIT_FAILURE);
        default:
            wait(NULL);
            if (close(fd_out) == -1) {
                perror("close, fd_out");
                exit(EXIT_FAILURE);
            }

            if (close(fd_erreur) == -1) {
                perror("close, fd_erreur");
                exit(EXIT_FAILURE);
            }

            break;
            if (close(fd_in) == -1) {
                perror("close, fd_in");
                exit(EXIT_FAILURE);
            }
    }

    if (close(fd_in) == -1) {
        perror("close, fd_in last");
        exit(EXIT_FAILURE);
    }

    return NULL;
}
