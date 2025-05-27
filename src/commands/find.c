//
// Created by Samuel on 05/05/2025.
//

#include "../../include/pignoufs.h"
#include "../../include/fs_structs.h"
#include "../../include/block_ops.h"
#include "../../include/inode_ops.h"
#include <string.h>
#include <ctype.h>

/**
 * Vérifie si une chaîne contient un motif
 * @param string Chaîne dans laquelle chercher
 * @param pattern Motif à rechercher
 * @return 1 si le motif est trouvé, 0 sinon
 */
int contains_pattern(const char *string, const char *pattern) {
    char haystack_lower[256];
    char needle_lower[256];

    // Copier et convertir string
    ssize_t i = 0;
    while (string[i] && i < 255) {
        haystack_lower[i] = (char) tolower(string[i]);
        i++;
    }
    haystack_lower[i] = '\0';

    // Copier et convertir pattern
    i = 0;
    while (pattern[i] && i < 255) {
        needle_lower[i] = (char) tolower(pattern[i]);
        i++;
    }
    needle_lower[i] = '\0';

    // Rechercher le motif
    return strstr(haystack_lower, needle_lower) != NULL;
}

int cmd_find(const char *fsname, const char *pattern) {
    fs_context_t ctx;
    int result = EXIT_SUCCESS;
    int found = 0;

    if (init_fs_context_and_verify(fsname, &ctx, O_RDONLY) < 0) {
        return EXIT_FAILURE;
    }

    printf("Recherche des fichiers contenant '%s'...\n", pattern);

    // Parcourir tous les inodes
    for (uint32_t i = 0; i < ctx.sb->max_inodes; i++) {
        block_t *inode_block = get_inode_block(ctx.fs_map, (int) i);
        if (!inode_block) {
            continue;
        }

        if (!verify_block_sha1(inode_block)) {
            fs_error("Attention: Le bloc d'inode %d est corrompu\n", i);
            continue;
        }

        inode_t *inode = (inode_t *) (inode_block->data);

        // Vérifier que l'inode correspond à un fichier existant
        if (!(inode->flags & PERM_EXISTS)) {
            continue;
        }

        // Vérifier si le nom du fichier contient le motif recherché
        if (contains_pattern(inode->filename, pattern)) {
            printf("Trouvé: %-20s Taille: %-8u Permissions: %c%c%c\n",
                   inode->filename,
                   inode->size,
                   (inode->flags & PERM_READ) ? 'r' : '-',
                   (inode->flags & PERM_WRITE) ? 'w' : '-',
                   (inode->flags & PERM_DIR) ? 'd' : '-');
            found++;
        }
    }

    if (found == 0) {
        printf("Aucun fichier correspondant au motif '%s' n'a été trouvé.\n", pattern);
    } else {
        printf("\nTotal: %d fichier(s) trouvé(s).\n", found);
    }

    // Libérer les ressources
    fs_free_context(&ctx);
    return result;
}