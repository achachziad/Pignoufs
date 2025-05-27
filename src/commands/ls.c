#include "../../include/pignoufs.h"
#include "../../include/fs_structs.h"
#include "../../include/block_ops.h"
#include "../../include/inode_ops.h"

int cmd_ls(const char *fsname, int argc, char **argv) {
    int detailed = 0;      // 0 = affichage simple, 1 = affichage détaillé
    const char *target_name = NULL;
    fs_context_t ctx;
    int result = EXIT_SUCCESS;

    // Analyse des options
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            detailed = 1;
        } else if (strncmp(argv[i], "//", 2) == 0) {
            target_name = argv[i] + 2; // enleve 1 seul slash
        }
    }

    // Initialiser le contexte du système de fichiers et vérifier sa validité
    if (init_fs_context_and_verify(fsname, &ctx, O_RDONLY) < 0) {
        return EXIT_FAILURE;
    }

    int found = 0;

    // Parcourir tous les inodes
    for (uint32_t i = 0; i < ctx.sb->max_inodes; i++) {
        block_t *inode_block = get_inode_block(ctx.fs_map, (int) i);
        if (!inode_block) {
            continue;
        }

        if (!verify_block_sha1(inode_block)) {
            fs_error("Erreur: Le bloc d'inode %d est corrompu\n", i);
            continue;
        }

        inode_t *inode = (inode_t *) (inode_block->data);

        // Afficher uniquement les inodes existants
        if (!(inode->flags & PERM_EXISTS)) {
            continue;
        }

        // Si un nom cible est spécifié, ne montrer que ce fichier
        if (!target_name || strcmp(inode->filename, target_name) == 0) {
            if (detailed) {
                printf("%-20s Taille: %-8u Permissions: %c%c%c\n",
                       inode->filename,
                       inode->size,
                       (inode->flags & PERM_READ) ? 'r' : '-',
                       (inode->flags & PERM_WRITE) ? 'w' : '-',
                       (inode->flags & PERM_DIR) ? 'd' : '-');
            } else {
                printf("%s\n", inode->filename);
            }
            found = 1;
            if (target_name) break; // si on cherchait un seul fichier, stop
        }
    }

    // Messages en cas d'absence de fichiers
    if (target_name && !found) {
        fs_error("Fichier %s non trouvé.\n", target_name);
        result = EXIT_FAILURE;
    } else if (!target_name && !found) {
        printf("Aucun fichier trouvé.\n");
    }

    // Libérer les ressources
    fs_free_context(&ctx);
    return result;
}