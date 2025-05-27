//
// Created by Samuel on 05/05/2025.
//

#ifndef PSA_PROJECT_DIR_OPS_H
#define PSA_PROJECT_DIR_OPS_H

#include "fs_structs.h"
#include "fs_common.h"

/**
 * Crée un répertoire dans le système de fichiers
 * @param ctx Contexte du système de fichiers
 * @param dirname Nom du répertoire à créer
 * @return Index de l'inode créé ou code d'erreur négatif
 */
int create_directory(fs_context_t *ctx, const char *dirname);

/**
 * Supprime un répertoire du système de fichiers
 * @param ctx Contexte du système de fichiers
 * @param dirname Nom du répertoire à supprimer
 * @return 0 en cas de succès, code d'erreur négatif sinon
 */
int remove_directory(fs_context_t *ctx, const char *dirname);

/**
 * Vérifie si un répertoire est vide
 * @param ctx Contexte du système de fichiers
 * @param dir_inode_index Index de l'inode du répertoire
 * @return 1 si le répertoire est vide, 0 sinon, code d'erreur négatif en cas d'erreur
 */
int is_directory_empty(fs_context_t *ctx, int dir_inode_index);

#endif // PSA_PROJECT_DIR_OPS_H