//
// Created by Samuel on 16/03/2025.
//

#include "fs_structs.h"

#ifndef PSA_PROJECT_BLOCK_OPS_H
#define PSA_PROJECT_BLOCK_OPS_H

// Structure pour passer des arguments aux threads de vérification
typedef struct {
    void *fs_map;            // Mapping du système de fichiers
    uint32_t start_block;    // Premier bloc à vérifier
    uint32_t end_block;      // Dernier bloc à vérifier
    int *corruption_found;   // Indicateur de corruption (partagé entre threads)
    pthread_mutex_t *mutex;  // Mutex pour protéger l'accès à corruption_found
} verify_thread_args_t;


/**
 * Calcule le SHA1 d'un bloc (sur les données uniquement pas l'en tete)
 * @param block Le block dont on veut calculer le sha1
 */
void compute_block_sha1(block_t *block);


/**
 * Verifie l'intégrité d'un block
 * @param block Le block dont on veut verifier l'intégrité
 * @return 0 si integre, 1 sinon
 */
int verify_block_sha1(block_t *block);

/**
 * Obtenir le block par son index
 * @param block_num Le numéro du block
 * @return L'adresse du block
 * */
block_t *get_block(void *addr, int block_index);

/**
 *
 * @param addr
 * @param block_num
 */
void set_block_used(void *addr, uint32_t block_num);

/**
 * Marquer un bloc comme libre
 * @param block_num Le numéro du block
 * */
void set_block_free(void *addr, uint32_t block_num);

/**
 * Trouve un bloc libre dans le système de fichiers
 * @param fs_map Pointeur vers la projection mémoire du système de fichiers
 * @param sb Pointeur vers le superbloc
 * @return Numéro du bloc libre ou 0 si aucun bloc libre n'est disponible
 */
uint32_t find_free_block(void *fs_map, superblock_t *sb);

/**
 * Incrémente le compteur de blocs libres dans le superbloc
 * @param fs_map Pointeur vers le mapping du système de fichiers
 */
void increment_free_blocks(void *fs_map);


void *verify_blocks_thread(void *arg);

int verify_fs_blocks_parallel(void *fs_map, superblock_t *sb, int num_threads);

int verify_inode_blocks_parallel(void *fs_map, int inode_index, int num_threads);

#endif //PSA_PROJECT_BLOCK_OPS_H

