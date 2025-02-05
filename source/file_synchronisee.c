#include "file_synchronisee.h"

#include <fcntl.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fonctions_utilitaires.h"

fileSynchronisee *initialiser_file(size_t taille_file) {
    if (taille_file <= TAILLE_MAX_FILE) {
        int fd_file = shm_open(NOM_SHM, O_RDWR | O_CREAT | O_TRUNC, 0642);
        if (fd_file == -1) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }

        if (ftruncate(fd_file, TAILLE_SHM) == -1) {
            perror("ftruncate");
            exit(EXIT_FAILURE);
        }

        fileSynchronisee *file = (fileSynchronisee *)mmap(NULL, TAILLE_SHM, PROT_READ | PROT_WRITE, MAP_SHARED, fd_file, 0);
        if (file == MAP_FAILED) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }

        if (close(fd_file) == -1) {
            perror("close");
            exit(EXIT_FAILURE);
        }

        file->taille = taille_file;
        file->tete = 0;
        file->queue = 0;

        if (sem_init(&file->vide, 1, (unsigned)taille_file) == -1) {
            perror("sem_init, file.vide");
            exit(EXIT_FAILURE);
        }

        if (sem_init(&file->plein, 1, 0) == -1) {
            perror("sem_init, file.plein");
            exit(EXIT_FAILURE);
        }

        if (sem_init(&file->mutex, 1, 1) == -1) {
            perror("sem_init, file.mutex");
            exit(EXIT_FAILURE);
        }

        file->estInitialisee = true;

        return file;
    } else {
        fprintf(stderr, "Erreur : Entrez une taille inférieure ou égale à 1000.\n");
        exit(EXIT_FAILURE);
    }
}

void enfiler(fileSynchronisee *file, pid_t pid_client) {
    if (!file->estInitialisee) {
        fprintf(stderr, "Erreur : la file n'est pas initialisée.\n");
        exit(EXIT_FAILURE);
    }
    if (!file_pleine(file)) {
        P(&file->vide);
        P(&file->mutex);

        file->donnees[file->tete] = pid_client;
        file->tete = (file->tete + 1) % file->taille;

        V(&file->mutex);
        V(&file->plein);

        printf("Enfilement de l'élément : %d\n", pid_client);
    } else {
        fprintf(stderr, "Ne peut pas enfiler un élément car la file est pleine.\n");
        exit(EXIT_FAILURE);
    }
}

pid_t defiler(fileSynchronisee *file) {
    if (!file->estInitialisee) {
        fprintf(stderr, "Erreur : la file n'est pas initialisée.\n");
        exit(EXIT_FAILURE);
    }
    if (!file_vide(file)) {
        pid_t pid_client;
        P(&file->plein);
        P(&file->mutex);

        pid_client = file->donnees[file->queue];
        file->queue = (file->queue + 1) % file->taille;

        V(&file->mutex);
        V(&file->vide);

        printf("Défilement de l'élément : %d\n", pid_client);
        return pid_client;
    } else {
        fprintf(stderr, "Ne peut pas défiler un élément car la file est vide.\n");
        exit(EXIT_FAILURE);
    }
}

void detruire_file(fileSynchronisee *file) {
    if (!file->estInitialisee) {
        fprintf(stderr, "Erreur : la file n'est pas initialisée.\n");
        exit(EXIT_FAILURE);
    }

    if (shm_unlink(NOM_SHM) == -1) {
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }

    if (sem_destroy(&file->vide) == -1) {
        perror("sem_destroy");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&file->plein) == -1) {
        perror("sem_destroy");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&file->mutex) == -1) {
        perror("sem_destroy");
        exit(EXIT_FAILURE);
    }

    file->estInitialisee = false;
}

bool file_vide(fileSynchronisee *file) {
    if (!file->estInitialisee) {
        fprintf(stderr, "Erreur : la file n'est pas initialisée.\n");
        exit(EXIT_FAILURE);
    }
    int valeur;
    sem_getvalue(&file->plein, &valeur);
    return valeur == 0;
}

bool file_pleine(fileSynchronisee *file) {
    if (!file->estInitialisee) {
        fprintf(stderr, "Erreur : la file n'est pas initialisée.\n");
        exit(EXIT_FAILURE);
    }
    int valeur;
    sem_getvalue(&file->vide, &valeur);
    return valeur == 0;
}

void ouvrir_file(fileSynchronisee **file) {
    int fd_file = shm_open(NOM_SHM, O_RDWR, 0642);
    if (fd_file == -1) {
        fprintf(stderr, "La file n'est pas initialisée.\nLe lanceur de commandes n'est pas en cours d'exécution.\n");
        exit(EXIT_FAILURE);
    }

    *file = (fileSynchronisee *)mmap(NULL, sizeof(fileSynchronisee), PROT_READ | PROT_WRITE, MAP_SHARED, fd_file, 0);
    if (file == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if (close(fd_file) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
}
