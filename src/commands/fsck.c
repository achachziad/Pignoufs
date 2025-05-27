#include "../../include/fs_common.h"
#include "../../include/block_ops.h"
#include <string.h>
#include <stdio.h>

/// 1. Vérifie le magic number
int check_magic(superblock_t *sb) {
    if (memcmp(sb->magic, "pignoufs", 8) != 0) {
        fs_error("Erreur : magic number invalide.\n");
        return -1;
    }
    return 0;
}

/// 2. Vérifie les SHA1 de tous les blocs
int check_sha1_all_blocks(fs_context_t *ctx) {
    int res = 0;
    for (uint32_t i = 0; i < ctx->sb->num_blocks; i++) {
        block_t *blk = get_block(ctx->fs_map, (int) i);
        if (!verify_block_sha1(blk)) {
            fs_error("Corruption SHA1 dans le bloc %u.\n", i);
            res = -1;
        }
    }
    return res;
}

/// 3. Vérifie la cohérence des types de blocs selon leur position
int check_block_types(fs_context_t *ctx) {
    int res = 0;
    uint32_t nbb = ctx->sb->num_blocks;

    for (uint32_t i = 0; i < nbb; i++) {
        block_t *blk = get_block(ctx->fs_map, (int) i);
        if (!blk) continue;
        uint32_t type = blk->type;

        if (i == 0 && type != BLOCK_TYPE_SUPERBLOCK) {
            fs_error("Bloc %u : attendu superbloc.\n", i);
            res = -1;

        } else if (i >= ctx->sb->bitmap_start && i < ctx->sb->inode_start && type != BLOCK_TYPE_BITMAP) {
            fs_error("Bloc %u : attendu bitmap.\n", i);
            res = -1;

        } else if (i >= ctx->sb->inode_start && i < ctx->sb->data_start) {
            if (type != BLOCK_TYPE_INODE && type != 0) { // inode ou libre
                fs_error("Bloc %u : attendu inode ou libre (type=%u).\n", i, type);
                res = -1;
            }

        } else if (i >= ctx->sb->data_start) {
            if (type != BLOCK_TYPE_DATA && type != BLOCK_TYPE_INDIRECT && type != 0) {
                fs_error("Bloc %u : type de données invalide (type=%u).\n", i, type);
                res = -1;
            }
        }
    }
    return res;
}

int is_block_free(fs_context_t *ctx, uint32_t i) {
    uint8_t *bitmap = (uint8_t *) ((char *) ctx->fs_map + ctx->sb->bitmap_start * BLOCK_SIZE);
    return !(bitmap[i / 8] & (1 << (i % 8)));
}

int check_bitmap_coherence(fs_context_t *ctx) {
    block_t *bitmap_block = get_block(ctx->fs_map, (int) ctx->sb->bitmap_start);
    if (!bitmap_block || !verify_block_sha1(bitmap_block)) {
        fs_error("Erreur : bloc bitmap invalide\n");
        return -1;
    }

    unsigned char *bitmap = bitmap_block->data;
    uint32_t count = 0;

    // On doit parcourir TOUS les blocs (du bloc 0 à num_blocks - 1)
    for (uint32_t i = 0; i < ctx->sb->num_blocks; i++) {
        int byte = (int) i / 8;
        int bit = (int) i % 8;

        // Si le bit est à 0 → bloc libre
        if ((bitmap[byte] & (1 << bit)) == 0) {
            count++;
        }
    }

    if (count  != ctx->sb->num_free_blocks) {
        fs_error("Incohérence bitmap : attendu %u libres, trouvé %u\n",
                 ctx->sb->num_free_blocks, count);
        return -1;
    }

    return 0;
}


/// 5. Réinitialise les verrous dans tous les blocs
void reset_all_locks(fs_context_t *ctx) {
    for (uint32_t i = 0; i < ctx->sb->num_blocks; i++) {
        block_t *blk = get_block(ctx->fs_map, (int) i);
        memset(blk->lock_read, 0, LOCK_SIZE);
        memset(blk->lock_write, 0, LOCK_SIZE);
    }
}

/// Entrée principale
int cmd_fsck(const char *fsname) {

    fs_context_t ctx;
    if (fs_init_context(fsname, &ctx, O_RDWR) < 0) {
        fs_error("Erreur : ouverture du conteneur '%s' impossible.\n", fsname);
        return -1;
    }

    int status = 0;

    if (check_magic(ctx.sb) < 0) status = -1;
    if (check_sha1_all_blocks(&ctx) < 0) status = -1;
    if (check_block_types(&ctx) < 0) status = -1;
    if (check_bitmap_coherence(&ctx) < 0) status = -1;

    reset_all_locks(&ctx);

    if (status == 0) {
        printf("Système de fichiers valide.\n");
    }

    fs_free_context(&ctx);
    return status;
}
