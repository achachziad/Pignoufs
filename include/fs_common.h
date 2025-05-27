//
// Created by Samuel on 04/05/2025.
//

#ifndef PSA_PROJECT_FS_COMMON_H
#define PSA_PROJECT_FS_COMMON_H

#include "pignoufs.h"
#include "fs_structs.h"

/**
 * Structure contenant les ressources du système de fichiers
 */
typedef struct {
    int fd;                 // Descripteur de fichier
    void *fs_map;           // Pointeur vers la projection mémoire
    ssize_t fs_size;         // Taille du fichier
    superblock_t *sb;       // Pointeur vers le superbloc
} fs_context_t;

/**
 * Initialise le contexte du système de fichiers
 * @param fsname Chemin vers le fichier conteneur
 * @param ctx Pointeur vers la structure de contexte à initialiser
 * @param mode Mode d'ouverture (O_RDONLY, O_RDWR)
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int fs_init_context(const char *fsname, fs_context_t *ctx, int mode);

/**
 * Libère les ressources du contexte
 * @param ctx Pointeur vers la structure de contexte
 */
void fs_free_context(fs_context_t *ctx);

/**
 * Vérifie la validité du système de fichiers
 * @param ctx Pointeur vers la structure de contexte
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int fs_verify(fs_context_t *ctx);

/**
 * Affiche un message d'erreur et retourne EXIT_FAILURE
 * @param format Format du message d'erreur
 * @param ... Arguments du format
 * @return -1
 */
int fs_error(const char *format, ...);

/**
 * Initialise le contexte du système de fichiers et vérifie sa validité
 * @param fsname Chemin vers le fichier conteneur
 * @param ctx Pointeur vers le contexte à initialiser
 * @param flags Flags d'ouverture (O_RDONLY, O_RDWR, etc.)
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int init_fs_context_and_verify(const char *fsname, fs_context_t *ctx, int flags);

#endif // PSA_PROJECT_FS_COMMON_H