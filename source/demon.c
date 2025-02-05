#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "file_synchronisee.h"
#include "fonctions_utilitaires.h"

fileSynchronisee *file;                // la file synchronisée
sem_t *semaphore;                      // pour l'activation du demon par un client
void gestionnaire_signal(int signum);  // pour quitter le programme avec succès

int main(int argc, char *argv[]) {
    // Calcul de la taille de la file
    int taille_file;
    if (argc == 1) {  // l'utilisateur a le choix d'entrer la taille dans le programme
        printf("Entrez la taille souhaitée de la file synchronisée : ");
        if (scanf("%d", &taille_file) != 1) {
            fprintf(stderr, "Erreur : Entrez un nombre valide.\n");
            exit(EXIT_FAILURE);
        }
    } else {  // ou bien l'utilisateur l'entre directement en paramètre lors de l'exécution
              // mais il faut vérifier que l'utilisateur a entré un nombre valide
        char *pointeur_fin;
        taille_file = (int)strtol(argv[1], &pointeur_fin, 10);
        if (*pointeur_fin != '\0') {
            fprintf(stderr, "Erreur : Entrez un nombre valide.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Gestion du signal pour quitter le programme
    // initialisation d'un sigaction
    struct sigaction action;
    action.sa_handler = gestionnaire_signal;
    action.sa_flags = 0;
    if (sigfillset(&action.sa_mask) == -1) {
        perror("sigfillset");
        exit(EXIT_FAILURE);
    }

    // association du sigaction à SIGINT (ctrl+c)
    if (sigaction(SIGINT, &action, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    // initialisation de la file avec la taille entrée
    file = initialiser_file((long unsigned int)taille_file);

    // ouverture du semaphore pour la synchronisation avec les clients
    semaphore = sem_open(NOM_SEMAPHORE, O_CREAT, 0644, 0);
    if (semaphore == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    // début de l'exécution du programme
    printf("Indication : pour quitter le programme, utilisez la commande ctrl+c.\n");
    printf("Le lanceur de commandes est en cours d'exécution...\n");

    // pour garder le programme actif jusqu'à la réception d'un signal SIGINT (ctrl+c)
    // l'utilisation du semaphore pour éviter que le programme consomme trop de ressouces du CPU
    while (1) {
        // Attendre qu'aumoins un client active le demon
        P(semaphore);

        // extraire le pid du client de la file synchronisée
        pid_t pid = defiler(file);
        pid_t *pid_client = (pid_t *)malloc(sizeof(pid_t));
        *pid_client = pid;

        printf("Exécution des commandes du client : %d...\n", pid);

        pthread_t thread;
        if (pthread_create(&thread, NULL, routine_thread, pid_client) != 0) {
            fprintf(stderr, " Erreur : création du thread.\n");
            exit(EXIT_FAILURE);
        }
        pthread_exit(NULL);

        printf("Les commandes du client : %d ont été exécutées.\n", pid);

        // Signaler au client que les commandes ont été exécutées
        V(semaphore);
    }

    /* la suppression du semaphore sera effectuée automatiquement lorsque le programme
        se termine avec SIGINT (ctrl+c) */
}

// le code a exécuter lors de la réception d'un SIGINT (ctrl+c)
void gestionnaire_signal(int signum) {
    if (signum < 0) {
        fprintf(stderr, "Numéro de signal incorrect.\n");
        exit(EXIT_FAILURE);
    }

    // on ferme le semaphore de synchronisation avant de fermer le programme
    if (sem_close(semaphore) == -1) {
        perror("sem_close");
        exit(EXIT_FAILURE);
    }
    if (sem_unlink(NOM_SEMAPHORE) == -1) {
        perror("sem_unlink");
        exit(EXIT_FAILURE);
    }

    detruire_file(file);

    printf("\nLe programme a été exécuté avec succès.\n");

    exit(EXIT_SUCCESS);
}