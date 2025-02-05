#ifndef FONCTIONS_UTILITAIRES_H
#define FONCTIONS_UTILITAIRES_H  // Pour une utilisation ultérieuree et améliorer la lisibilité du code

#include "file_synchronisee.h"

#define NOM_SEMAPHORE "/semaphore"
#define TAILLE 256

// pour la file synchronisée
void P(sem_t *sem);  // sem_wait
void V(sem_t *sem);  // sem_post

// pour le lanceur de commandes (demon)
void extraire_commandes(char *chaine_de_characteres, char ***commandes, int *nombre_commandes);  // pour extraire les commandes à partir d'une chaine de charactères
void *routine_thread(void *arg);                                                                 // la routine que va exécuter le thread

#endif
