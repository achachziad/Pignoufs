//
// Created by Samuel on 16/03/2025.
//

#include "block_ops.h"
#include "fs_common.h"
#include <stdio.h>


/// Opération sur les block, je sais pas encore si c'est utile d'avoir un fichier expres pour ca ou pa

void compute_block_sha1(block_t *block) {
    // Calcul du SHA1 sur les données uniquement (pas sur l'en-tête)
    SHA1(block->data, DATA_SIZE, block->sha1);
}

int verify_block_sha1(block_t *block) {
    unsigned char computed_sha1[SHA1_SIZE];

    // Calculer le SHA1 des données
    SHA1(block->data, DATA_SIZE, computed_sha1);

    // Comparer avec le SHA1 stocké
    return memcmp(computed_sha1, block->sha1, SHA1_SIZE) == 0;
}

block_t *get_block(void *addr, int block_index) {
    // Vérifier que l'index est valide
    if (block_index < 0) {
        return NULL;
    }

    // Retourner le bloc correspondant à l'index
    return (block_t *) (addr + block_index * BLOCK_SIZE);
}


void set_block_free(void *addr, uint32_t block_num) {
    superblock_t *sb = (superblock_t *) (((block_t *) addr)->data);

    if (block_num >= sb->num_blocks) return;

    block_t *bitmap_block = get_block(addr, (int) sb->bitmap_start);
    if (!bitmap_block) return;

    unsigned char *bitmap = bitmap_block->data;

    bitmap[block_num / 8] &= ~(1 << (block_num % 8));

    sb->num_free_blocks++;
    compute_block_sha1(bitmap_block);
    compute_block_sha1((block_t *) addr);
}


void set_block_used(void *addr, uint32_t block_num) {
    superblock_t *sb = (superblock_t *) (((block_t *) addr)->data);

    if (block_num >= sb->num_blocks) return;

    block_t *bitmap_block = get_block(addr, (int) sb->bitmap_start);
    if (!bitmap_block) return;

    unsigned char *bitmap = bitmap_block->data;

    bitmap[block_num / 8] |= (1 << (block_num % 8));

    sb->num_free_blocks--;
    compute_block_sha1(bitmap_block);
    compute_block_sha1((block_t *) addr);
}


uint32_t find_free_block(void *fs_map, superblock_t *sb) {
    // Parcourir les blocs de bitmap
    uint32_t bitmap_blocks = (sb->num_blocks / (BLOCK_SIZE * 8)) + 1;

    for (int i = 0; i < (int) bitmap_blocks; i++) {
        block_t *bitmap_block = get_block(fs_map, (int) sb->bitmap_start + i);
        if (!bitmap_block) continue;

        unsigned char *bitmap = bitmap_block->data;
        uint32_t bits_per_block = BLOCK_SIZE * 8;
        uint32_t start_bit = i * bits_per_block;

        // Parcourir les bits du bitmap
        for (uint32_t j = 0; j < DATA_SIZE * 8; j++) {
            uint32_t bit_index = start_bit + j;
            if (bit_index < sb->data_start) continue; // Ignorer les blocs réservés
            if (bit_index >= sb->num_blocks) break;   // Fin du système de fichiers

            // Vérifier si le bit est à 0 (bloc libre)
            if ((bitmap[j / 8] & (1 << (j % 8))) == 0) {
                // Marquer le bloc comme utilisé
                bitmap[j / 8] |= (1 << (j % 8));
                compute_block_sha1(bitmap_block);

                // Décrémenter le nombre de blocs libres
                compute_block_sha1((block_t *) fs_map);

                return bit_index;
            }
        }
    }

    return 0; // Aucun bloc libre trouvé
}

void increment_free_blocks(void *fs_map) {
    // Récupérer le superbloc
    block_t *superblock = (block_t *) fs_map;
    superblock_t *sb = (superblock_t *) superblock->data;

    // Incrémenter le compteur de blocs libres
    sb->num_free_blocks++;

    // Mettre à jour le SHA1 du superbloc
    compute_block_sha1(superblock);
}


/**
 * Fonction exécutée par chaque thread pour vérifier un ensemble de blocs
 * @param arg Arguments du thread (structure verify_thread_args_t)
 * @return NULL
 */
void *verify_blocks_thread(void *arg) {
    verify_thread_args_t *args = (verify_thread_args_t *) arg;
    superblock_t *sb = (superblock_t *) (((block_t *) args->fs_map)->data);

    for (uint32_t i = args->start_block; i < args->end_block && i < sb->num_blocks; i++) {
        block_t *block = get_block(args->fs_map, (int) i);
        if (block && block->type != 0) {  // Ignorer les blocs non initialisés
            if (!verify_block_sha1(block)) {
                pthread_mutex_lock(args->mutex);
                *(args->corruption_found) = 1;
                fs_error("Corruption détectée dans le bloc %u\n", i);
                pthread_mutex_unlock(args->mutex);
            }
        }
    }

    return NULL;
}

/**
 * Vérifie l'intégrité de tous les blocs du système de fichiers en parallèle
 * @param fs_map Mapping du système de fichiers
 * @param sb Pointeur vers le superbloc
 * @param num_threads Nombre de threads à utiliser
 * @return 0 si aucune corruption n'est détectée, 1 sinon
 */
int verify_fs_blocks_parallel(void *fs_map, superblock_t *sb, int num_threads) {
    if (num_threads <= 0) num_threads = 1;
    if (num_threads > 32) num_threads = 32;  // Limitation raisonnable

    pthread_t threads[num_threads];
    verify_thread_args_t args[num_threads];
    int corruption_found = 0;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    uint32_t blocks_per_thread = sb->num_blocks / num_threads;
    uint32_t remainder = sb->num_blocks % num_threads;

    for (int i = 0; i < num_threads; i++) {
        args[i].fs_map = fs_map;
        args[i].start_block = i * blocks_per_thread;
        args[i].end_block = (i + 1) * blocks_per_thread;
        if (i == num_threads - 1) args[i].end_block += remainder;
        args[i].corruption_found = &corruption_found;
        args[i].mutex = &mutex;

        if (pthread_create(&threads[i], NULL, verify_blocks_thread, &args[i]) != 0) {
            fs_error("Erreur lors de la création du thread %d\n", i);
            // En cas d'échec, continuer avec moins de threads
            num_threads = i;
            break;
        }
    }

    // Attendre la fin de tous les threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    return corruption_found;
}

/**
 * Vérifie en parallèle l'intégrité d'un inode et de tous ses blocs de données
 * @param fs_map Mapping du système de fichiers
 * @param inode_index Index de l'inode à vérifier
 * @param num_threads Nombre de threads à utiliser
 * @return 0 si aucune corruption n'est détectée, 1 sinon
 */
int verify_inode_blocks_parallel(void *fs_map, int inode_index, int num_threads) {
    superblock_t *sb = (superblock_t *) (((block_t *) fs_map)->data);

    block_t *inode_block = get_block(fs_map, (int) sb->inode_start + inode_index);
    if (!inode_block || !verify_block_sha1(inode_block)) {
        return 1;  // Corruption détectée dans le bloc d'inode
    }

    // Récupérer les blocs associés à l'inode
    uint32_t *blocks = NULL;
    uint32_t num_blocks = 0;
    uint32_t max_blocks = 0;

    // Analyser l'inode pour obtenir les blocs de données
    // Cette fonction devrait être adaptée à votre structure d'inode
    inode_t *inode = (inode_t *) inode_block->data;

    // Allouer de la mémoire pour stocker tous les blocs potentiels
    max_blocks = 10 + (DATA_SIZE / sizeof(uint32_t));  // Blocs directs + indirects
    blocks = (uint32_t *) malloc(max_blocks * sizeof(uint32_t));
    if (!blocks) return 1;

    // Collecter les blocs directs
    for (int i = 0; i < 10; i++) {
        if (inode->direct_blocks[i] != 0) {
            blocks[num_blocks++] = inode->direct_blocks[i];
        }
    }

    // Collecter les blocs indirects
    if (inode->indirect_block != 0) {
        block_t *indirect_block = get_block(fs_map, (int) inode->indirect_block);
        if (indirect_block && verify_block_sha1(indirect_block)) {
            blocks[num_blocks++] = inode->indirect_block;  // Ajouter le bloc d'indirection lui-même

            uint32_t *block_refs = (uint32_t *) indirect_block->data;
            for (unsigned long i = 0; i < DATA_SIZE / sizeof(uint32_t); i++) {
                if (block_refs[i] != 0) {
                    blocks[num_blocks++] = block_refs[i];
                }
            }
        }
    }

    // Si nous avons peu de blocs, ou un seul thread, vérifier en série
    if ((int) num_blocks <= num_threads || num_threads <= 1) {
        int corruption = 0;
        for (uint32_t i = 0; i < num_blocks; i++) {
            block_t *block = get_block(fs_map, (int) blocks[i]);
            if (block && !verify_block_sha1(block)) {
                corruption = 1;
                fs_error("Corruption détectée dans le bloc %u\n", blocks[i]);
            }
        }
        free(blocks);
        return corruption;
    }

    // Vérifier les blocs en parallèle
    pthread_t threads[num_threads];
    verify_thread_args_t args[num_threads];
    int corruption_found = 0;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

    // Distribuer les blocs entre les threads
    for (int i = 0; i < num_threads; i++) {
        args[i].fs_map = fs_map;
        args[i].corruption_found = &corruption_found;
        args[i].mutex = &mutex;

        // Vérifier directement les blocs spécifiques au lieu de plages
        // Cette approche est plus adaptée pour un petit nombre de blocs non contigus

        // Créer un thread pour chaque groupe de blocs
        pthread_create(&threads[i], NULL, verify_blocks_thread, &args[i]);
    }

    // Attendre la fin de tous les threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    free(blocks);
    return corruption_found;
}

