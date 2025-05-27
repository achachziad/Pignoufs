//
// Created by Samuel on 21/03/2025.
//

#include "fs_structs.h"

#ifndef PSA_PROJECT_FS_UTILS_H
#define PSA_PROJECT_FS_UTILS_H

/**
 * Ajouter un verrou
 * @param fd
 * @param inode_number
 * @param lock_type
 * @return 0 si le verrou a pu etre ajouté
 */
int lock_file(int fd, int inode_number, int lock_type);

/**
 * Supprimer un verrou
 * @param fd
 * @param inode_number
 * @return 0 si le verrou a pu etre supprimé
 */
int unlock_file(int fd, int inode_number);

/**
 * Verrouiller un fichier en lecture
 * @param fd
 * @param inode_number
 * @return 0 si le verrou a été ajouté
 */
int read_lock_file(int fd, int inode_number);

/**
 * Verrouiller un fichir en écriture
 * @param fd
 * @param inode_number
 * @return 0 si le verrou a été ajouté
 */
int write_lock_file(int fd, int inode_number);

#endif //PSA_PROJECT_FS_UTILS_H

