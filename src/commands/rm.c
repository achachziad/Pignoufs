#include "../../include/pignoufs.h"
#include "../../include/fs_structs.h"
#include "../../include/block_ops.h"
#include "../../include/inode_ops.h"

int cmd_rm(const char *fsname, const char *filename) {
    if (filename == NULL) {
        return fs_error("Usage: rm <fichier>");
    }

    fs_context_t ctx;
    int result = EXIT_FAILURE;

    // Initialiser le contexte du système de fichiers et vérifier sa validité
    if (init_fs_context_and_verify(fsname, &ctx, O_RDWR) < 0) {
        return EXIT_FAILURE;
    }

    // Gérer le chemin avec éventuellement des slashes au début
    const char *pignoufs_path = filename;
    if (strncmp(pignoufs_path, "//", 2) == 0) {
        pignoufs_path += 2;
    }

    // Trouver l'inode du fichier à supprimer
    int inode_idx = find_inode_by_name(ctx.fs_map, pignoufs_path);
    if (inode_idx == -1) {
        fs_error("Erreur : Fichier '%s' introuvable", pignoufs_path);
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    // Récupérer le bloc d'inode
    block_t *inode_block = get_inode_block(ctx.fs_map, inode_idx);
    if (!inode_block) {
        fs_error("Erreur : Impossible d'accéder au bloc d'inode");
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    if (!verify_block_sha1(inode_block)) {
        fs_error("Erreur : Bloc d'inode corrompu");
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    pthread_mutex_lock((pthread_mutex_t *) inode_block->lock_write);

    inode_t *inode = (inode_t *) inode_block->data;

    // Vérifier que le fichier existe
    if (!(inode->flags & PERM_EXISTS)) {
        fs_error("Erreur : Le fichier '%s' n'existe pas", pignoufs_path);
        pthread_mutex_unlock((pthread_mutex_t *) inode_block->lock_write);
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    // Libérer tous les blocs directs
    for (int i = 0; i < 10; i++) {
        if (inode->direct_blocks[i] != 0) {
            set_block_free(ctx.fs_map, inode->direct_blocks[i]);
            inode->direct_blocks[i] = 0;
        }
    }

    // Libérer les blocs indirects si besoin
    if (inode->indirect_block != 0) {
        block_t *indirect_block = get_block(ctx.fs_map, (int) inode->indirect_block);
        if (indirect_block && verify_block_sha1(indirect_block)) {
            pthread_mutex_lock((pthread_mutex_t *) indirect_block->lock_write);
            uint32_t *indirect_pointers = (uint32_t *) indirect_block->data;
            int max_indirect = DATA_SIZE / sizeof(uint32_t);

            for (int i = 0; i < max_indirect; i++) {
                if (indirect_pointers[i] != 0) {
                    set_block_free(ctx.fs_map, indirect_pointers[i]);
                }
            }
            pthread_mutex_unlock((pthread_mutex_t *) indirect_block->lock_write);
        }
        set_block_free(ctx.fs_map, inode->indirect_block);
        inode->indirect_block = 0;
    }

    // Libérer l'inode
    inode->flags = 0;
    inode->size = 0;
    memset(inode->filename, 0, sizeof(inode->filename));

    compute_block_sha1(inode_block);

    pthread_mutex_unlock((pthread_mutex_t *) inode_block->lock_write);

    // Mettre à jour le SHA1 du superbloc
    compute_block_sha1((block_t *) ctx.fs_map);

    printf("Fichier '%s' supprimé avec succès.\n", pignoufs_path);
    result = EXIT_SUCCESS;

    // Libérer les ressources
    fs_free_context(&ctx);
    return result;
}