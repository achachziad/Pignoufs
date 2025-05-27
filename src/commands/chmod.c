//
// Created by Samuel on 16/03/2025.
//

#include "pignoufs.h"
#include "fs_structs.h"
#include "block_ops.h"
#include "inode_ops.h"

int cmd_chmod(const char *fsname, const char *filename, const char *mode) {
    fs_context_t ctx;

    if (init_fs_context_and_verify(fsname, &ctx, O_RDWR) < 0) {
        return EXIT_FAILURE;
    }

    if (strncmp(filename, "//", 2) == 0) {
        filename += 2;
    }

    // Rechercher l'inode correspondant au fichier et vérifier les permissions
    int inode_index = find_file_with_perm_check(&ctx, filename, PERM_READ);
    if (inode_index < 0) {
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    block_t *inode_block = get_inode_block(ctx.fs_map, inode_index);
    if (!inode_block || !verify_block_sha1(inode_block)) {
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    pthread_mutex_lock((pthread_mutex_t *) inode_block->lock_write);

    inode_t *inode = (inode_t *) inode_block->data;

    if (strcmp(mode, "+r") == 0) {
        inode->flags |= PERM_READ;
    } else if (strcmp(mode, "-r") == 0) {
        inode->flags &= ~PERM_READ;
    } else if (strcmp(mode, "+w") == 0) {
        inode->flags |= PERM_WRITE;
    } else if (strcmp(mode, "-w") == 0) {
        inode->flags &= ~PERM_WRITE;
    } else {
        pthread_mutex_unlock((pthread_mutex_t *) inode_block->lock_write);
        fs_error("Mode invalide. Utiliser +r, -r, +w ou -w\n");
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    compute_block_sha1(inode_block);

    pthread_mutex_unlock((pthread_mutex_t *) inode_block->lock_write);

    if (msync(ctx.fs_map, (int) ctx.fs_size, MS_SYNC) < 0) {
        perror("Erreur msync");
    }

    fs_free_context(&ctx);

    printf("Permissions de '%s' mises à jour (%s)\n", filename, mode);
    return EXIT_SUCCESS;
}
