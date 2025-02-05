#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "file_synchronisee.h"
#include "fonctions_utilitaires.h"

#define TAILLE_BUFFER 2048

int main(int argc, char *argv[]) {
    if (argc > 1) {
        // Ouverture de la file
        fileSynchronisee *file = NULL;
        ouvrir_file(&file);

        // Enfilement
        enfiler(file, getpid());

        // Création de des tubes
        char *tube_entree = malloc(sizeof(char) * 20);
        snprintf(tube_entree, 20, "tube_%d_entree", getpid());

        char *tube_sortie = malloc(sizeof(char) * 20);
        snprintf(tube_sortie, 20, "tube_%d_sortie", getpid());

        char *tube_erreur = malloc(sizeof(char) * 20);
        snprintf(tube_erreur, 20, "tube_%d_erreur", getpid());

        mkfifo(tube_entree, 0642);
        mkfifo(tube_sortie, 0642);
        mkfifo(tube_erreur, 0642);

        // Signaler au demon pour exécuter les commandes
        sem_t *semaphore = sem_open(NOM_SEMAPHORE, O_CREAT, 0644, 0);
        if (semaphore == SEM_FAILED) {
            perror("sem_open");
            return 1;
        }
        V(semaphore);

        // Ouverture du tube d'entrée
        int fd_tube;
        fd_tube = open(tube_entree, O_WRONLY);
        if (fd_tube == -1) {
            perror("open, tube_entree");
            exit(EXIT_FAILURE);
        }

        // Écriture des commandes dans le tube d'entrée
        if (write(fd_tube, argv[1], strlen(argv[1])) == -1) {
            perror("write, tube_entree");
            exit(EXIT_FAILURE);
        }

        // Fermeture du tube d'entrée (dissocier le fd du tube)
        close(fd_tube);
        printf("Ecriture dans le tube d'entrée effectuée avec succès\n");

        // Ouverture du tube de sortie
        fd_tube = open(tube_sortie, O_RDONLY);
        if (fd_tube == -1) {
            perror("open, tube_sortie");
            exit(EXIT_FAILURE);
        }

        // Attendre que le demon exécute les commandes
        P(semaphore);

        char buffer[TAILLE_BUFFER];
        ssize_t n;

        // Lecture depuis le tube de sortie
        while ((n = read(fd_tube, buffer, TAILLE_BUFFER)) >= 0) {
            if (n == 0) {  // tube de sortie vide = erreur dans l'exécution de la commande
                // Fermeture du tube de sortie (dissocier le fd du tube)
                close(fd_tube);
                // Overture du tube d'erreurs
                fd_tube = open(tube_erreur, O_RDONLY);
                if (fd_tube == -1) {
                    perror("open, tube_erreur");
                    exit(EXIT_FAILURE);
                }

                // Lecture depuis le tube d'erreurs
                while ((n = read(fd_tube, buffer, TAILLE_BUFFER)) > 0) {
                    if (write(STDERR_FILENO, buffer, (size_t)n) < n) {
                        perror("write, tube_erreur");
                        exit(EXIT_FAILURE);
                    }
                }
                break;

            } else {
                if (write(STDOUT_FILENO, buffer, (size_t)n) < n) {
                    perror("write, tube_sortie");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // Fermeture du tube de sortie/erreur(dissocier le fd du tube)
        close(fd_tube);
        printf("Lecutre depuis le tube de sortie effectuée avec succès\n");
        unlink(tube_entree);
        unlink(tube_sortie);
        unlink(tube_erreur);

        // Fermeture du semaphore de synchronisation
        if (sem_close(semaphore) == -1) {
            perror("sem_close");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("Veuillez entrer au moins une commande entre deux \" \".\n");
    }
    return EXIT_SUCCESS;
}
