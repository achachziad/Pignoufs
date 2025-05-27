//
// Created by Samuel on 29/04/2025.
//

///SI je dis pas de bétises toutes les commandes auront besoin de ca
#include "../../include/pignoufs.h"
#include "../../include/fs_structs.h"
#include "../../include/block_ops.h"
#include "../../include/inode_ops.h"
#include "../../include/fs_common.h"


int cmd_cat(const char *fsname, const char *filename) {
    fs_context_t ctx;
    int result = EXIT_FAILURE;
    char *buffer = NULL;
    uint32_t buffer_size = 0;

    // Initialiser le contexte du système de fichiers et vérifier sa validité
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

    // Lire le contenu du fichier
    if (read_inode_content(&ctx, inode_index, &buffer, &buffer_size) < 0) {
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    // Afficher le contenu du fichier
    if (buffer_size > 0) {
        if (write(STDOUT_FILENO, buffer, buffer_size) != buffer_size) {
            perror("Erreur lors de l'écriture sur la sortie standard");
            result = EXIT_FAILURE;
        } else {
            result = EXIT_SUCCESS;
        }
    } else {
        // Fichier vide
        result = EXIT_SUCCESS;
    }

    // Libérer les ressources
    free(buffer);
    fs_free_context(&ctx);
    return result;
}