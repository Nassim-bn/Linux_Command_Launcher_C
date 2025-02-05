#ifndef FILE_SYNCHRONISEE_H
#define FILE_SYNCHRONISEE_H  // Pour la structure de la file synchronisée

#include <semaphore.h>
#include <stdbool.h>
#include <stdlib.h>

#define TAILLE_MAX_FILE 1000

struct fileSynchronisee {
    pid_t donnees[TAILLE_MAX_FILE];  // stock les pid des clients
    size_t taille;                   // la taille de la file entrée par l'utilisateur
    size_t tete;                     // la tête de la file
    size_t queue;                    // la queue de la file
    sem_t vide;                      // le nombre de places vides disponibles dans la file
    sem_t plein;                     // le nombre d'éléments présents dans la file
    sem_t mutex;                     // un sémaphore pour l'exclusion mutuelle
    bool estInitialisee;             /* l'état de la file, initialisée ou pas,
                                        doit être vrai pour pouvoir utiliser les autres commandes */
};

typedef struct fileSynchronisee fileSynchronisee;  // pour améliorer la lisibilité du code

#define NOM_SHM "/file_shm"                  // le nom de la mémoire partagée
#define TAILLE_SHM sizeof(fileSynchronisee)  // la taille de la mémoire partagée

fileSynchronisee *initialiser_file(size_t taille_file);  // initialise la file en mémoire partagée avec la taille entrée en paramètre
void enfiler(fileSynchronisee *file, pid_t pid_client);  // enfile le pid d'un client
pid_t defiler(fileSynchronisee *file);                   // défile le pid du client et le retourne
void detruire_file(fileSynchronisee *file);              // supprime la file
bool file_vide(fileSynchronisee *file);                  // renvoie vrai si la file est vide, faux sinon
bool file_pleine(fileSynchronisee *file);                // renvoie vrai si la file est pleine, faux sinon
void ouvrir_file(fileSynchronisee **file);               // pour ouvrir une file déjà existante (par le client)

#endif
