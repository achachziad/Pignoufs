//
// Created by Samuel on 16/03/2025.
//

#ifndef PSA_PROJECT_INODE_OPS_H
#define PSA_PROJECT_INODE_OPS_H

#include "fs_structs.h"
#include "fs_common.h"

/**
 * Trouver l'index d'un inode par son nom
 * @param filename Nom de l'inode
 * @return Index de l'inode
 * */
int find_inode_by_name(void *addr, const char *filename);


/**
 * Obtenir l'adresse d'un inode
 * @param inode_index Index de l'inode
 * @return le block contenant l'inode
 * */
block_t *get_inode_block(void *addr, int inode_index);

/**
 * Marquer un inode comme libre
 * @param inode_index Index de l'inode
 * */
void set_inode_free(void *addr, int inode_index);

/** Vérifier les permissions
 * @param perm Permissions à vérifier
 * */
int check_permissions(inode_t *inode, uint32_t perm);

/**
 * Lit le contenu complet d'un fichier à partir de son inode
 * @param ctx Contexte du système de fichiers
 * @param inode_index Index de l'inode à lire
 * @param buffer Pointeur vers un buffer qui sera alloué pour stocker les données
 * @param size Pointeur vers une variable qui recevra la taille des données lues
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int read_inode_content(fs_context_t *ctx, int inode_index, char **buffer, uint32_t *size);

/**
 * Écrit des données dans un fichier à partir de son inode
 * @param ctx Contexte du système de fichiers
 * @param inode_index Index de l'inode à utiliser
 * @param data Données à écrire
 * @param size Taille des données à écrire
 * @param append Mode d'écriture (0: écrasement, 1: ajout)
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int write_inode_content(fs_context_t *ctx, int inode_index, const char *data, uint32_t size, int append);

/**
 * Crée un nouveau fichier ou réinitialise un fichier existant
 * @param ctx Contexte du système de fichiers
 * @param filename Nom du fichier à créer/réinitialiser
 * @param check_write Vérifier les permissions d'écriture si le fichier existe
 * @return Index de l'inode créé/réinitialisé ou -1 en cas d'erreur
 */
int create_or_reset_file(fs_context_t *ctx, const char *filename, int check_write);

/**
 * Trouve un fichier par son nom et vérifie les permissions d'accès
 * @param ctx Contexte du système de fichiers
 * @param filename Nom du fichier à rechercher
 * @param check_perm Permission à vérifier (PERM_READ, PERM_WRITE, etc.)
 * @return Index de l'inode trouvé ou -1 en cas d'erreur
 */
int find_file_with_perm_check(fs_context_t *ctx, const char *filename, uint32_t check_perm);

/**
 * Cette méthode n'est pas implémenter
 * @param ctx
 * @param inode_index
 * @param buffer
 * @param size
 * @return
 */
int read_inode_content_threaded(fs_context_t *ctx, int inode_index, char **buffer, uint32_t *size);

/**
 * Cette méthode n'est pas implémenter
 * @param ctx
 * @param inode_index
 * @param buffer
 * @param size
 * @return
 */
int write_inode_content_threaded(fs_context_t *ctx, int inode_index, char **buffer, uint32_t *size);

#endif //PSA_PROJECT_INODE_OPS_H