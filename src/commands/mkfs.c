#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include "pignoufs.h"
#include "fs_structs.h"
#include "block_ops.h"
#include "fs_common.h"

void init_block_lock(block_t *block) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init((pthread_mutex_t *) block->lock_read, &attr);
    pthread_mutex_init((pthread_mutex_t *) block->lock_write, &attr);
    pthread_mutexattr_destroy(&attr);
}


int cmd_mkfs(const char *fsname, int nb_inode, int nb_block) {
    int bitmap_blocks = ((nb_inode + nb_block) / (BLOCK_SIZE * 8)) + 1;
    int nbb = 1 + bitmap_blocks + nb_inode + nb_block; // Nombre total de blocs

    int fd = open(fsname, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        fs_error("Erreur lors de l'ouverture du fichier conteneur");
        return EXIT_FAILURE;
    }

    if (ftruncate(fd, nbb * BLOCK_SIZE) < 0) {
        fs_error("Erreur lors de l'ajustement de la taille du fichier conteneur");
        close(fd);
        return EXIT_FAILURE;
    }

    void *fs_map = mmap(NULL, nbb * BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fs_map == MAP_FAILED) {
        fs_error("Erreur lors de la projection mémoire");
        close(fd);
        return EXIT_FAILURE;
    }

    // Initialisation du superbloc
    block_t *superbloc_block = (block_t *) fs_map;
    // 1. Nettoyer tout le bloc (important)
    memset(superbloc_block, 0, sizeof(block_t));

    // 2. Remplir le DATA : écrire magic, block_size, num_blocks, ...
    superblock_t *superbloc = (superblock_t *) (superbloc_block->data);
    memset(superbloc, 0, sizeof(superblock_t));
    memcpy(superbloc->magic, "pignoufs", 8);
    superbloc->block_size = BLOCK_SIZE;
    superbloc->num_blocks = nbb;
    superbloc->num_free_blocks = nb_block;
    superbloc->bitmap_start = 1;
    superbloc->inode_start = 1 + bitmap_blocks;
    superbloc->data_start = superbloc->inode_start + nb_inode;
    superbloc->max_inodes = nb_inode;

    init_block_lock(superbloc_block);

    unsigned char sha1[20];
    SHA1(superbloc_block->data, 4000, sha1);
    memcpy(superbloc_block->sha1, sha1, 20);

    // 4. Puis mettre type = 1
    superbloc_block->type = 1;



    // Initialiser les bitmaps
    char *bitmap_area = (char *) (fs_map + BLOCK_SIZE);
    memset(bitmap_area, 0, bitmap_blocks * BLOCK_SIZE);

    // Les blocs 0 à (superbloc + bitmaps + inodes) sont alloués
    for (uint32_t i = 0; i < superbloc->data_start; i++) {
        bitmap_area[i / 8] |= (1 << (i % 8));
    }

    // Calculer SHA1 pour chaque bloc de bitmap
    for (int i = 0; i < bitmap_blocks; i++) {
        block_t *bitmap_block = (block_t *) (fs_map + (1 + i) * BLOCK_SIZE);

        // SHA1 et metadata
        SHA1(bitmap_block->data, 4000, sha1);
        memcpy(bitmap_block->sha1, sha1, 20);
        bitmap_block->type = 2; // Bitmap
        memset(bitmap_block->lock_read, 0, LOCK_SIZE);
        memset(bitmap_block->lock_write, 0, LOCK_SIZE);
        init_block_lock(bitmap_block);

    }

    // Initialiser les inodes
    block_t *inode_area = (block_t *) (fs_map + (1 + bitmap_blocks) * BLOCK_SIZE);
    for (int i = 0; i < nb_inode; i++) {
        block_t *inode_block = &inode_area[i];
        memset(inode_block, 0, sizeof(block_t));  // CLEAN total

        init_block_lock(inode_block);  // Initialiser mutex AVANT tout

        inode_block->type = 3;

        // SHA1
        compute_block_sha1(inode_block);
    }

     // Initialiser les blocs de données (DATA) pas sur de bien faire
     block_t *data_area = (block_t *) (fs_map + superbloc->data_start * BLOCK_SIZE);
     for (int i = 0; i < nb_block; i++) {
         block_t *data_block = &data_area[i];
         memset(data_block, 0, sizeof(block_t));

         init_block_lock(data_block);
         data_block->type = BLOCK_TYPE_DATA;

         compute_block_sha1(data_block);
     }

    printf(" Système de fichiers %s initialisé avec %d inodes et %d blocs allouables.\n", fsname, nb_inode, nb_block);

    if (munmap(fs_map, nbb * BLOCK_SIZE) < 0) {
        fs_error("Erreur lors de la libération de la projection mémoire");
    }

    close(fd);

    printf("nbb (total blocs) = %d\n", nbb);
    printf("bitmap_blocks = %d\n", bitmap_blocks);
    printf("nb_inodes = %d\n", nb_inode);
    printf("nb_blocks allouables = %d\n", nb_block);

    return EXIT_SUCCESS;
}


