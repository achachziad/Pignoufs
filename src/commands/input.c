//
// Created by Samuel on 04/05/2025.
//

#include "../../include/pignoufs.h"
#include "../../include/fs_structs.h"
#include "../../include/block_ops.h"
#include "../../include/inode_ops.h"
#include "../../include/fs_common.h"

int cmd_input(const char *fsname, const char *filename) {
    fs_context_t ctx;
    char *buffer = NULL;
    uint32_t buffer_size = DATA_SIZE;

    // Allouer un buffer pour lire l'entrée standard
    buffer = malloc(buffer_size);
    if (!buffer) {
        return fs_error("Erreur d'allocation mémoire");
    }

    // Initialiser le contexte du système de fichiers et vérifier sa validité
    if (init_fs_context_and_verify(fsname, &ctx, O_RDWR) < 0) {
        free(buffer);
        return EXIT_FAILURE;
    }

    if (strncmp(filename, "//", 2) == 0) {
        filename += 2;
    }

    // Créer ou réinitialiser le fichier
    int inode_index = create_or_reset_file(&ctx, filename, 1);
    if (inode_index < 0) {
        free(buffer);
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    // Lire l'entrée standard et écrire dans le fichier
    ssize_t total_bytes = 0;
    ssize_t bytes_read;
    int append = 0;  // Premier écrit en mode écrasement

    // Boucle pour lire l'entrée standard
    while ((bytes_read = read(STDIN_FILENO, buffer, buffer_size)) > 0) {
        if (write_inode_content(&ctx, inode_index, buffer, bytes_read, append) < 0) {
            fs_error("Erreur lors de l'écriture dans le fichier '%s'", filename);
            free(buffer);
            fs_free_context(&ctx);
            return EXIT_FAILURE;
        }
        total_bytes += bytes_read;
        append = 1;  // Les écritures suivantes sont en mode ajout
    }

    if (bytes_read < 0) {
        fs_error("Erreur lors de la lecture de l'entrée standard");
        free(buffer);
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    printf("Données écrites avec succès dans '%s' (%zu octets)\n", filename, total_bytes);

    // Libérer les ressources
    free(buffer);
    fs_free_context(&ctx);
    return EXIT_SUCCESS;
}