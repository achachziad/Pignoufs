//
// Created by Samuel on 16/03/2025.
//

#include "pignoufs.h"
#include "fs_structs.h"
#include "block_ops.h"
#include "inode_ops.h"
#include <signal.h>

static pthread_mutex_t *global_mutex = NULL; // pour unlock dans signal handler

void signal_handler(int signum) {
    if (global_mutex != NULL) {
        pthread_mutex_unlock(global_mutex);
        printf("\nVerrou libéré (signal %d reçu)\n", signum);
    }
    exit(EXIT_SUCCESS);
}

int cmd_lock(const char *fsname, const char *filename, const char *mode) {
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

    pthread_mutex_t *mutex = strcmp(mode, "w") == 0 ? (pthread_mutex_t *) inode_block->lock_write
                                                    : (pthread_mutex_t *) inode_block->lock_read;

    // 4. Préparer le signal handler
    global_mutex = mutex;
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler); // Attrape aussi Ctrl+C

    // 5. Essayer de prendre le verrou
    printf("Tentative de prise de verrou...\n");

    if (pthread_mutex_lock(mutex) != 0) {
        perror("Erreur pthread_mutex_lock");
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    printf("Verrou pris.\n");
    printf("Attente d'un signal (Ctrl+C ou kill -SIGTERM) pour libérer...\n");

    pause();

    pthread_mutex_unlock(mutex);
    fs_free_context(&ctx);
    return EXIT_SUCCESS;
}
