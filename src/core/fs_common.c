//
// Created by Samuel on 04/05/2025.
//

#include "../../include/fs_common.h"
#include "../../include/block_ops.h"
#include <stdarg.h>

int fs_init_context(const char *fsname, fs_context_t *ctx, int mode) {
    // Initialiser le contexte avec des valeurs par défaut
    memset(ctx, 0, sizeof(fs_context_t));
    ctx->fd = -1;
    ctx->fs_map = NULL;

    // Ouvrir le fichier conteneur
    ctx->fd = open(fsname, mode);
    if (ctx->fd < 0) {
        perror("Erreur lors de l'ouverture du fichier conteneur");
        return -1;
    }

    // Récupérer la taille du fichier
    struct stat st;
    if (fstat(ctx->fd, &st) < 0) {
        perror("Erreur lors de la récupération des informations du fichier");
        close(ctx->fd);
        ctx->fd = -1;
        return -1;
    }
    ctx->fs_size = st.st_size;

    // Projeter le fichier en mémoire
    int prot = (mode == O_RDONLY) ? PROT_READ : (PROT_READ | PROT_WRITE);
    ctx->fs_map = mmap(NULL, ctx->fs_size, prot, MAP_SHARED, ctx->fd, 0);
    if (ctx->fs_map == MAP_FAILED) {
        perror("Erreur lors de la projection mémoire");
        close(ctx->fd);
        ctx->fd = -1;
        ctx->fs_map = NULL;
        return -1;
    }

    // Accéder au superbloc
    block_t *superblock = (block_t *) ctx->fs_map;
    ctx->sb = (superblock_t *) superblock->data;

    return 0;
}

void fs_free_context(fs_context_t *ctx) {
    if (ctx) {
        // Libérer la projection mémoire
        if (ctx->fs_map && ctx->fs_map != MAP_FAILED) {
            munmap(ctx->fs_map, (int) ctx->fs_size);
        }

        // Fermer le descripteur de fichier
        if (ctx->fd >= 0) {
            close(ctx->fd);
        }

        // Réinitialiser le contexte
        memset(ctx, 0, sizeof(fs_context_t));
        ctx->fd = -1;
    }
}

int fs_verify(fs_context_t *ctx) {
    if (!ctx || !ctx->fs_map || !ctx->sb) {
        return -1;
    }

    block_t *superblock = (block_t *) ctx->fs_map;

    // Vérifier la signature magique
    if (memcmp(ctx->sb->magic, "pignoufs", 8) != 0) {
        fs_error("Fichier conteneur non valide (signature incorrecte)\n");
        return -1;
    }

    // Vérifier l'intégrité du superbloc
    if (!verify_block_sha1(superblock)) {
        fs_error("Fichier conteneur corrompu (superbloc)\n");
        return -1;
    }

    return 0;
}

int fs_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    return -1;
}

int init_fs_context_and_verify(const char *fsname, fs_context_t *ctx, int flags) {
    if (fs_init_context(fsname, ctx, flags) < 0) {
        return -1;
    }

    if (fs_verify(ctx) < 0) {
        fs_free_context(ctx);
        return -1;
    }

    return 0;
}
