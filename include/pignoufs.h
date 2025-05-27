//
// Created by Samuel on 16/03/2025.
//

#ifndef PSA_PROJECT_PIGNOUFS_H
#define PSA_PROJECT_PIGNOUFS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#include <openssl/sha.h>
#include <pthread.h>
#include <stddef.h>

// def des constantes
#define BLOCK_SIZE         4168
#define DATA_SIZE          4000
#define SHA1_SIZE          20
#define TYPE_SIZE          4
#define LOCK_SIZE          72

// Types de blocs
#define BLOCK_TYPE_SUPERBLOCK 1
#define BLOCK_TYPE_BITMAP     2
#define BLOCK_TYPE_INODE      3
#define BLOCK_TYPE_DATA       4
#define BLOCK_TYPE_INDIRECT   5

// Codes d'erreurs
// (jsp trop encore si on en a besoin, mais c'est souvent présent dans les projets que j'ai vu)
#define FS_SUCCESS          0
#define FS_ERROR_OPEN      -1
#define FS_ERROR_MAP       -2
#define FS_ERROR_FORMAT    -3
#define FS_ERROR_CHECKSUM  -4
#define FS_ERROR_FULL      -5
#define FS_ERROR_NOTFOUND  -6
#define FS_ERROR_PERMISSION -7

// Permission flags
#define PERM_EXISTS 0x1
#define PERM_READ 0x2
#define PERM_WRITE 0x4
#define PERM_LOCK_READ 0x8
#define PERM_LOCK_WRITE 0x10
#define PERM_DIR 0x20
#define PERM_EXEC 0x40

#define UNUSED(x) (void)(x)
#endif //PSA_PROJECT_PIGNOUFS_H
