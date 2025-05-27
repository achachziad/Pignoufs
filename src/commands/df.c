
//
// Created by Samuel on 16/03/2025.
//
#include "../../include/pignoufs.h"
#include "../../include/fs_structs.h"
#include "../../include/block_ops.h"
#include "fs_common.h"


int cmd_df(const char *fsname) {
    fs_context_t ctx;

    if (init_fs_context_and_verify(fsname, &ctx, O_RDONLY) < 0) {
        return EXIT_FAILURE;
    }

    // Lecture du superbloc
    superblock_t *superbloc = (superblock_t *) ctx.fs_map;
    printf("Système de fichiers : %s\n", fsname);
    printf("Taille d'un bloc : %d octets\n", superbloc->block_size);
    printf("Nombre total de blocs : %d\n", superbloc->num_blocks);
    printf("Nombre de blocs libres : %d\n", superbloc->num_free_blocks);
    printf("Nombre maximal d'inodes : %d\n", superbloc->max_inodes);
    printf("Espace libre estimé : %d Ko\n", (superbloc->num_free_blocks * superbloc->block_size) / 1024);

    // Libérer les ressources
    fs_free_context(&ctx);
    return EXIT_SUCCESS;
}
