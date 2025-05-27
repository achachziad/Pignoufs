//
// Created by Samuel on 16/03/2025.
//

#ifndef PSA_PROJECT_FS_STRUCTS_H
#define PSA_PROJECT_FS_STRUCTS_H

#include "pignoufs.h"

// Structure d'un bloc générique
typedef struct {
    unsigned char data[DATA_SIZE];
    unsigned char sha1[SHA1_SIZE];
    uint32_t type;
    //ALL TYPES
    // 1. superbloc
    //  2. bitmap /
    //  3. inode /
    //  4. bloc allouable libre
    //  5. bloc de données
    //  6. bloc d’indirection simple
    //  7. bloc d’indirection double
    unsigned char lock_read[LOCK_SIZE];
    unsigned char lock_write[LOCK_SIZE];
} block_t;

// Structure du superbloc
typedef struct {
    char magic[8];              // Nombre magique (signature)
    uint32_t block_size;         // Taille d'un bloc (4096)
    uint32_t num_blocks;         // Nombre total de blocs
    uint32_t num_free_blocks;    // Nombre de blocs libres
    uint32_t bitmap_start;       // Premier bloc de bitmap
    uint32_t inode_start;        // Premier bloc d'inodes
    uint32_t data_start;         // Premier bloc de données
    uint32_t max_inodes;         // Nombre maximal d'inodes
} superblock_t;

// Structure d'un inode
typedef struct {
    uint32_t flags;               // Bit 0: existe, Bit 1: lecture, Bit 2: écriture, Bit 3: verrou lecture, Bit 4: verrou écriture, Bit 5: répertoire
    uint32_t mode;               // Droits d'accès
    uint32_t size;               // Taille du fichier en octets
    uint32_t direct_blocks[10];  // Pointeurs directs vers blocs de données
    uint32_t indirect_block;     // Pointeur vers bloc d'indirection
    char filename[256];          // Nom du fichier
} inode_t;

typedef struct {
    uint32_t inode_index;     // Index de l'inode associé à cette entrée
    char name[256];           // Nom de l'entrée (fichier ou répertoire)
    uint8_t type;             // Type d'entrée (0: fichier, 1: répertoire)
} dir_entry_t;

#endif //PSA_PROJECT_FS_STRUCTS_H
