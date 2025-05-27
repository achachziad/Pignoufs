//
// Created by Samuel on 16/03/2025.
//


/// Toute les fonctions type 'utils' pour le projet pourrait etre ici

#include "../../include/fs_utils.h"

int lock_file(int fd, int inode_number, int lock_type) {
    struct flock fl;

    // Configurer le verrou
    fl.l_type = (short) lock_type;           // F_RDLCK, F_WRLCK, F_UNLCK
    fl.l_whence = SEEK_SET;          // Début du fichier comme référence
    fl.l_start = inode_number;       // Utiliser l'inode comme identifiant unique
    fl.l_len = 1;                    // Verrou sur un seul octet (symbolique)

    // Tenter d'obtenir le verrou
    return fcntl(fd, F_SETLK, &fl);  // Non-bloquant
}

int unlock_file(int fd, int inode_number) {
    return lock_file(fd, inode_number, F_UNLCK);
}

int read_lock_file(int fd, int inode_number) {
    return lock_file(fd, inode_number, F_RDLCK);
}

int write_lock_file(int fd, int inode_number) {
    return lock_file(fd, inode_number, F_WRLCK);
}