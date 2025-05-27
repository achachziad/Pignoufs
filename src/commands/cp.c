//
// Created by Samuel on 16/03/2025.
//

#include "../../include/pignoufs.h"
#include "../../include/fs_structs.h"
#include "../../include/block_ops.h"
#include "../../include/inode_ops.h"
#include "../../include/fs_common.h"

/**
 * Copier un fichier de Pignoufs vers le système de fichiers réel
 * @param ctx Contexte du système de fichiers
 * @param pignoufs_path Chemin du fichier dans Pignoufs
 * @param ext_path Chemin du fichier de destination
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int copy_from_pignoufs(fs_context_t *ctx, const char *pignoufs_path, const char *ext_path) {
    // Trouver l'inode du fichier source avec vérification des permissions
    int inode_index = find_file_with_perm_check(ctx, pignoufs_path, PERM_READ);
    if (inode_index < 0) {
        return -1;  // L'erreur a déjà été affichée
    }

    // Récupérer le bloc d'inode
    block_t *inode_block = get_inode_block(ctx->fs_map, inode_index);
    if (!inode_block || !verify_block_sha1(inode_block)) {
        return fs_error("Erreur lors de l'accès à l'inode ou inode corrompu");
    }

    // Récupérer l'inode
    inode_t *inode = (inode_t *) inode_block->data;

    // Ouvrir le fichier destination
    int dst_fd = open(ext_path, O_WRONLY | O_CREAT | O_TRUNC, inode->mode ? inode->mode : 0644);
    if (dst_fd == -1) {
        return fs_error("Erreur lors de l'ouverture du fichier destination");
    }

    // Utiliser read_inode_content pour lire le contenu du fichier
    char *buffer = NULL;
    uint32_t buffer_size = 0;

    if (read_inode_content(ctx, inode_index, &buffer, &buffer_size) < 0) {
        close(dst_fd);
        return -1;  // L'erreur a déjà été affichée
    }

    // Écrire le contenu dans le fichier de destination
    ssize_t bytes_written = write(dst_fd, buffer, buffer_size);
    if (bytes_written != buffer_size) {
        free(buffer);
        close(dst_fd);
        return fs_error("Erreur lors de l'écriture dans le fichier destination");
    }

    // Libérer les ressources
    free(buffer);
    close(dst_fd);

    printf("Fichier '%s' copié avec succès vers '%s' (%u octets)\n",
           pignoufs_path, ext_path, buffer_size);
    return 0;
}

/**
 * Copier un fichier du système de fichiers réel vers Pignoufs
 * @param ctx Contexte du système de fichiers
 * @param ext_path Chemin du fichier source dans le système de fichiers réel
 * @param pignoufs_path Chemin du fichier de destination dans Pignoufs
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int copy_to_pignoufs(fs_context_t *ctx, const char *ext_path, const char *pignoufs_path) {
    // Vérifier l'existence du fichier source
    struct stat src_stat;
    if (stat(ext_path, &src_stat) == -1) {
        return fs_error("Le fichier source '%s' n'existe pas", ext_path);
    }

    // Vérifier que le fichier source est un fichier régulier
    if (!S_ISREG(src_stat.st_mode)) {
        return fs_error("La source '%s' doit être un fichier régulier", ext_path);
    }

    // Ouvrir le fichier source
    int src_fd = open(ext_path, O_RDONLY);
    if (src_fd == -1) {
        return fs_error("Impossible d'ouvrir le fichier source '%s'", ext_path);
    }

    // Créer ou réinitialiser le fichier destination dans Pignoufs
    int inode_index = create_or_reset_file(ctx, pignoufs_path, 1);
    if (inode_index < 0) {
        close(src_fd);
        return -1;  // L'erreur a déjà été affichée
    }

    // Allouer un buffer pour lire le fichier source
    char *buffer = malloc(src_stat.st_size);
    if (!buffer && src_stat.st_size > 0) {
        close(src_fd);
        return fs_error("Erreur d'allocation mémoire pour le buffer");
    }

    // Lire le contenu du fichier source
    ssize_t bytes_read = read(src_fd, buffer, src_stat.st_size);
    if (bytes_read != src_stat.st_size) {
        free(buffer);
        close(src_fd);
        return fs_error("Erreur lors de la lecture du fichier source ");
    }

    // Fermer le fichier source
    close(src_fd);

    // Écrire le contenu dans le fichier destination de Pignoufs
    if (write_inode_content(ctx, inode_index, buffer, bytes_read, 0) < 0) {
        free(buffer);
        return -1;  // L'erreur a déjà été affichée
    }

    // Mettre à jour le mode du fichier
    block_t *inode_block = get_inode_block(ctx->fs_map, inode_index);
    if (inode_block && verify_block_sha1(inode_block)) {
        inode_t *inode = (inode_t *) inode_block->data;
        inode->mode = src_stat.st_mode & 0777;  // Copier les permissions du fichier source
        compute_block_sha1(inode_block);
    }

    // Libérer le buffer
    free(buffer);

    printf("Fichier '%s' copié avec succès vers '%s' (%ld octets)\n",
           ext_path, pignoufs_path, bytes_read);
    return 0;
}

/**
 * Fonction de commande cp pour être appelée par le main
 * @param fsname Nom du système de fichiers
 * @param source Chemin source
 * @param destination Chemin destination
 * @return Code d'erreur
 */
int cmd_cp(const char *fsname, const char *source, const char *destination) {
    int result = EXIT_FAILURE;
    int from_pignoufs = 0;
    int to_pignoufs = 0;
    fs_context_t ctx;

    // Déterminer la direction de la copie
    if (source[0] == '/' && source[1] == '/') {
        from_pignoufs = 1;
        source += 2;  // Sauter le préfixe //
    }
    if (destination[0] == '/' && destination[1] == '/') {
        to_pignoufs = 1;
        destination += 2;  // Sauter le préfixe //
    }

    // Vérifier qu'une source ou destination est dans Pignoufs
    if (!from_pignoufs && !to_pignoufs) {
        return fs_error("La source ou la destination doit être dans Pignoufs (préfixée par '//')");
    }

    // Initialiser le contexte du système de fichiers et vérifier sa validité
    if (init_fs_context_and_verify(fsname, &ctx, O_RDWR) < 0) {
        return EXIT_FAILURE;
    }

    // Effectuer la copie en fonction de la direction
    if (from_pignoufs) {
        result = copy_from_pignoufs(&ctx, source, destination);
    } else {
        result = copy_to_pignoufs(&ctx, source, destination);
    }

    // Libérer le contexte du système de fichiers
    fs_free_context(&ctx);

    return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}