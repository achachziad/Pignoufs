//
// Created by Samuel on 05/05/2025.
//


#include "../../include/dir_ops.h"
#include "../../include/inode_ops.h"
#include "../../include/block_ops.h"

int create_directory(fs_context_t *ctx, const char *dirname) {
    // Vérifie si le répertoire existe déjà
    int inode_index = find_inode_by_name(ctx->fs_map, dirname);
    if (inode_index >= 0) {
        // Le répertoire existe déjà, vérifier s'il s'agit bien d'un répertoire
        block_t *inode_block = get_inode_block(ctx->fs_map, inode_index);
        inode_t *inode = (inode_t *) inode_block->data;

        if (!(inode->flags & PERM_DIR)) {
            return FS_ERROR_NOTFOUND; // Un fichier avec le même nom existe déjà
        }
        return FS_ERROR_NOTFOUND; // Le répertoire existe déjà
    }

    // Crée un nouveau inode pour le répertoire
    inode_index = create_or_reset_file(ctx, dirname, 0);
    if (inode_index < 0) {
        return inode_index; // Erreur lors de la création de l'inode
    }

    // Configure l'inode comme un répertoire
    block_t *inode_block = get_inode_block(ctx->fs_map, inode_index);
    inode_t *inode = (inode_t *) inode_block->data;
    inode->flags |= PERM_DIR; // Marque comme répertoire
    inode->size = 0;          // Taille initiale à zéro

    // Met à jour le checksum du bloc inode
    compute_block_sha1(inode_block);

    return inode_index;
}

int remove_directory(fs_context_t *ctx, const char *dirname) {
    // Trouve l'inode du répertoire
    int inode_index = find_inode_by_name(ctx->fs_map, dirname);
    if (inode_index < 0) {
        return FS_ERROR_NOTFOUND; // Répertoire non trouvé
    }

    // Vérifie que c'est bien un répertoire
    block_t *inode_block = get_inode_block(ctx->fs_map, inode_index);
    inode_t *inode = (inode_t *) inode_block->data;

    if (!(inode->flags & PERM_DIR)) {
        return FS_ERROR_NOTFOUND; // Ce n'est pas un répertoire
    }

    // Vérifie que le répertoire est vide
    if (!is_directory_empty(ctx, inode_index)) {
        return FS_ERROR_NOTFOUND; // Le répertoire n'est pas vide
    }

    // Libère les blocs de données associés au répertoire
    for (int i = 0; i < 10; i++) {
        if (inode->direct_blocks[i] != 0) {
            set_block_free(ctx->fs_map, inode->direct_blocks[i]);
            increment_free_blocks(ctx->fs_map);
        }
    }

    // Gère le bloc d'indirection si présent
    if (inode->indirect_block != 0) {
        block_t *indirect_block = get_block(ctx->fs_map, (int) inode->indirect_block);
        uint32_t *indirect_ptrs = (uint32_t *) indirect_block->data;

        for (unsigned long i = 0; i < DATA_SIZE / sizeof(uint32_t); i++) {
            if (indirect_ptrs[i] != 0) {
                set_block_free(ctx->fs_map, indirect_ptrs[i]);
                increment_free_blocks(ctx->fs_map);
            }
        }

        set_block_free(ctx->fs_map, inode->indirect_block);
        increment_free_blocks(ctx->fs_map);
    }

    // Libère l'inode
    set_inode_free(ctx->fs_map, inode_index);

    return FS_SUCCESS;
}


int is_directory_empty(fs_context_t *ctx, int dir_inode_index) {
    block_t *dir_inode_block = get_inode_block(ctx->fs_map, dir_inode_index);
    inode_t *dir_inode = (inode_t *) dir_inode_block->data;

    // Vérifie que l'inode est bien un répertoire
    if (!(dir_inode->flags & PERM_DIR)) {
        return FS_ERROR_NOTFOUND; // Ce n'est pas un répertoire
    }

    // Si la taille est 0, le répertoire est vide
    if (dir_inode->size == 0) {
        return 1;
    }

    // Sinon, on vérifie le contenu
    char *dir_content = NULL;
    uint32_t dir_size = 0;
    int result = read_inode_content(ctx, dir_inode_index, &dir_content, &dir_size);
    if (result != FS_SUCCESS) {
        return result; // Erreur lors de la lecture
    }

    // Libère la mémoire et retourne le résultat
    free(dir_content);

    // Le répertoire est vide si sa taille est 0 ou s'il ne contient aucune entrée
    return (dir_size == 0) ? 1 : 0;
}
