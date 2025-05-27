//
// Created by Samuel on 16/03/2025.
//

#include "inode_ops.h"
#include "block_ops.h"


int find_inode_by_name(void *addr, const char *filename) {
    // Accéder correctement au superbloc
    superblock_t *sb = (superblock_t *) (((block_t *) addr)->data);

    // Parcourir tous les blocs d'inodes
    for (uint32_t i = 0; i < sb->max_inodes; i++) {
        block_t *inode_block = (block_t *) (addr + (sb->inode_start + i) * BLOCK_SIZE);
        inode_t *inode = (inode_t *) inode_block->data;

        // Vérifier si l'inode existe et correspond au nom
        if ((inode->flags & PERM_EXISTS) && strcmp(inode->filename, filename) == 0) {
            return (int) i;
        }
    }
    return -1;
}

block_t *get_inode_block(void *addr, int inode_index) {
    // Accéder correctement au superbloc
    superblock_t *sb = (superblock_t *) (((block_t *) addr)->data);

    // Vérifier que l'index est valide
    if (inode_index < 0 || inode_index >= (int) sb->max_inodes) {
        return NULL;
    }

    // Retourner le bloc correspondant à l'inode
    return (block_t *) (addr + (sb->inode_start + inode_index) * BLOCK_SIZE);
}

void set_inode_free(void *addr, int inode_index) {
    // Récupérer le bloc d'inode
    block_t *inode_block = get_inode_block(addr, inode_index);
    if (!inode_block) {
        return;
    }

    // Réinitialiser l'inode
    inode_t *inode = (inode_t *) inode_block->data;
    memset(inode, 0, sizeof(inode_t));

    // Mise à jour du SHA1 du bloc d'inode
    compute_block_sha1(inode_block);
}


int check_permissions(inode_t *inode, uint32_t perm) {
    return (inode->flags & perm) == perm;
}


/**
 * Lit le contenu complet d'un fichier à partir de son inode
 * @param ctx Contexte du système de fichiers
 * @param inode_index Index de l'inode à lire
 * @param buffer Pointeur vers un buffer qui sera alloué pour stocker les données
 * @param size Pointeur vers une variable qui recevra la taille des données lues
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int read_inode_content(fs_context_t *ctx, int inode_index, char **buffer, uint32_t *size) {
    // Initialiser les variables de sortie
    *buffer = NULL;
    *size = 0;

    block_t *inode_block = get_inode_block(ctx->fs_map, inode_index);
    if (!inode_block || !verify_block_sha1(inode_block)) {
        return fs_error("Erreur lors de l'accès à l'inode ou inode corrompu");
    }

    int result = 0;
    int mutex_locked = 0;

    pthread_mutex_t *mutex_ptr = (pthread_mutex_t *) inode_block->lock_read;

    // On ne verrouille jamais pour la lecture, on vérifie juste s'il est libre
    // Si le trylock réussit, cela signifie qu'aucune écriture n'est en cours
    int lock_result = pthread_mutex_trylock(mutex_ptr);

    if (lock_result == 0) {
        // Le verrou était libre, on l'a obtenu
        // On le libère immédiatement car on n'en a pas besoin pour la lecture
        pthread_mutex_unlock(mutex_ptr);
    } else {
        // Le verrou est occupé, on attend
        int wait_result = pthread_mutex_lock(mutex_ptr);
        if (wait_result != 0) {
            result = fs_error("Erreur lors du verrouillage");
            goto cleanup;
        }
    }

    // Variable pour suivre si on a verrouillé avec succès
    mutex_locked = 1;

    // Récupérer les informations de l'inode
    inode_t *inode = (inode_t *) inode_block->data;

    // Vérifier les permissions et l'existence
    if (!(inode->flags & PERM_EXISTS)) {
        result = fs_error("Le fichier n'existe pas");
        goto cleanup;
    }

    if (!check_permissions(inode, PERM_READ)) {
        result = fs_error("Permission de lecture refusée");
        goto cleanup;
    }

    // Allouer un buffer pour stocker le contenu complet du fichier
    *size = inode->size;
    if (*size == 0) {
        // Si le fichier est vide, retourner un buffer vide mais valide
        *buffer = malloc(1);  // Allouer au moins 1 octet
        if (!*buffer) {
            result = fs_error("Erreur d'allocation mémoire");
            goto cleanup;
        }
        (*buffer)[0] = '\0';
        goto cleanup;  // Succès avec un fichier vide
    }

    *buffer = malloc(*size);
    if (!*buffer) {
        result = fs_error("Erreur d'allocation mémoire");
        goto cleanup;
    }

    uint32_t bytes_read = 0;
    uint32_t remaining = inode->size;

    // Parcourir les blocs directs
    for (int i = 0; i < 10 && remaining > 0; i++) {
        uint32_t block_num = inode->direct_blocks[i];
        if (block_num == 0) {
            break;  // Fin des blocs directs
        }

        block_t *data_block = get_block(ctx->fs_map, (int) block_num);
        if (!data_block || !verify_block_sha1(data_block)) {
            result = fs_error("Erreur lors de l'accès au bloc de données %d ou bloc corrompu", block_num);
            goto cleanup;
        }

        uint32_t to_read = (remaining < DATA_SIZE) ? remaining : DATA_SIZE;
        memcpy(*buffer + bytes_read, data_block->data, to_read);

        bytes_read += to_read;
        remaining -= to_read;
    }

    // Traiter le bloc d'indirection si nécessaire
    if (remaining > 0 && inode->indirect_block != 0) {
        // Récupérer le bloc d'indirection
        block_t *indirect_block = get_block(ctx->fs_map, (int) inode->indirect_block);
        if (!indirect_block || !verify_block_sha1(indirect_block)) {
            result = fs_error("Erreur lors de l'accès au bloc d'indirection ou bloc corrompu");
            goto cleanup;
        }

        // Parcourir les références aux blocs de données dans le bloc d'indirection
        uint32_t *block_refs = (uint32_t *) indirect_block->data;
        int max_refs = DATA_SIZE / sizeof(uint32_t);  // Nombre max de références

        for (int i = 0; i < max_refs && remaining > 0; i++) {
            uint32_t block_num = block_refs[i];
            if (block_num == 0) {
                break;  // Fin des blocs référencés
            }

            block_t *data_block = get_block(ctx->fs_map, (int) block_num);
            if (!data_block || !verify_block_sha1(data_block)) {
                result = fs_error("Erreur lors de l'accès au bloc de données %d (indirect) ou bloc corrompu",
                                  block_num);
                goto cleanup;
            }

            uint32_t to_read = (remaining < DATA_SIZE) ? remaining : DATA_SIZE;
            memcpy(*buffer + bytes_read, data_block->data, to_read);

            bytes_read += to_read;
            remaining -= to_read;
        }
    }

    // Si toutes les données n'ont pas été lues avec succès
    if (bytes_read != inode->size) {
        result = fs_error("Attention: seulement %u octets lus sur %u", bytes_read, inode->size);
    }

    cleanup:
    // Déverrouiller le mutex avant de quitter
    if (mutex_locked) {
        pthread_mutex_unlock(mutex_ptr);
    }

    // Nettoyer le buffer si une erreur s'est produite
    if (result != 0 && *buffer != NULL) {
        free(*buffer);
        *buffer = NULL;
        *size = 0;
    }

    return result;
}

/**
 * Écrit des données dans un fichier à partir de son inode
 * @param ctx Contexte du système de fichiers
 * @param inode_index Index de l'inode à utiliser
 * @param data Données à écrire
 * @param size Taille des données à écrire
 * @param append Mode d'écriture (0: écrasement, 1: ajout)
 * @return 0 en cas de succès, -1 en cas d'erreur
 */
int write_inode_content(fs_context_t *ctx, int inode_index, const char *data, uint32_t size, int append) {
    block_t *inode_block = get_inode_block(ctx->fs_map, inode_index);
    if (!inode_block || !verify_block_sha1(inode_block)) {
        return fs_error("Erreur lors de l'accès à l'inode ou inode corrompu");
    }

    int result = 0;
    int mutex_locked = 0;

    inode_t *inode = (inode_t *) inode_block->data;
    pthread_mutex_t *mutex_ptr = (pthread_mutex_t *) inode_block->lock_write;
    pthread_mutex_t *mutex_ptr_r = (pthread_mutex_t *) inode_block->lock_read;

    pthread_mutex_lock(mutex_ptr_r);

    int lock_result = pthread_mutex_lock(mutex_ptr);
    if (lock_result != 0) {
        result = fs_error("Erreur lors du lock");
        goto cleanup;
    }
    mutex_locked = 1;

    if (!(inode->flags & PERM_EXISTS)) {
        result = fs_error("Le fichier n'existe pas");
        goto cleanup;
    }

    if (!check_permissions(inode, PERM_WRITE)) {
        result = fs_error("Permission d'écriture refusée");
        goto cleanup;
    }

    uint32_t original_size = append ? inode->size : 0;
    uint32_t total_size = original_size + size;

    uint32_t current_blocks = (original_size + DATA_SIZE - 1) / DATA_SIZE;
    uint32_t total_blocks = (total_size + DATA_SIZE - 1) / DATA_SIZE;
    uint32_t blocks_needed = total_blocks - current_blocks;

    if (blocks_needed > ctx->sb->num_free_blocks) {
        result = fs_error("Espace insuffisant sur le système de fichiers");
        goto cleanup;
    }

    uint32_t bytes_written = 0, remaining = size;

    int direct_blocks_used = 0;
    for (int i = 0; i < 10; i++) {
        if (inode->direct_blocks[i] != 0) direct_blocks_used++;
        else break;
    }

    uint32_t last_block_position = 0;
    if (append && original_size > 0) {
        last_block_position = original_size % DATA_SIZE;
    }

    if (append && original_size > 0 && last_block_position > 0) {
        uint32_t block_num;

        if (direct_blocks_used < 10) {
            block_num = inode->direct_blocks[direct_blocks_used - 1];
        } else {
            block_t *indirect_block = get_block(ctx->fs_map, (int) inode->indirect_block);
            if (!indirect_block || !verify_block_sha1(indirect_block)) {
                result = fs_error("Erreur lors de l'accès au bloc d'indirection ou bloc corrompu");
                goto cleanup;
            }

            uint32_t *block_refs = (uint32_t *) indirect_block->data;
            int indirect_index = (int) (original_size - 10 * DATA_SIZE + DATA_SIZE - 1) / DATA_SIZE - 1;
            block_num = block_refs[indirect_index];
        }

        block_t *last_block = get_block(ctx->fs_map, (int) block_num);
        if (!last_block || !verify_block_sha1(last_block)) {
            result = fs_error("Erreur lors de l'accès au dernier bloc ou bloc corrompu");
            goto cleanup;
        }

        uint32_t space_left = DATA_SIZE - last_block_position;
        uint32_t to_write = (remaining < space_left) ? remaining : space_left;

        memcpy(last_block->data + last_block_position, data, to_write);
        compute_block_sha1(last_block);

        bytes_written += to_write;
        remaining -= to_write;
    }

    int block_index = append ? direct_blocks_used : 0;

    while (remaining > 0 && block_index < 10) {
        uint32_t block_num;

        if (!append || block_index >= direct_blocks_used) {
            block_num = find_free_block(ctx->fs_map, ctx->sb);
            if (block_num == 0) {
                result = fs_error("Erreur lors de l'allocation d'un bloc direct");
                goto cleanup;
            }

            set_block_used(ctx->fs_map, block_num);
            printf("[DEBUG] Bloc alloué (direct): %u\n", block_num);
            inode->direct_blocks[block_index] = block_num;
        } else {
            block_num = inode->direct_blocks[block_index];
        }

        block_t *data_block = get_block(ctx->fs_map, (int) block_num);
        if (!data_block) {
            result = fs_error("Erreur lors de l'accès à un bloc direct");
            goto cleanup;
        }

        if (!append || block_index >= direct_blocks_used) {
            memset(data_block->data, 0, DATA_SIZE);
            data_block->type = BLOCK_TYPE_DATA;
        }

        uint32_t to_write = (remaining < DATA_SIZE) ? remaining : DATA_SIZE;
        memcpy(data_block->data, data + bytes_written, to_write);
        compute_block_sha1(data_block);

        bytes_written += to_write;
        remaining -= to_write;
        block_index++;
    }

    if (remaining > 0) {
        block_t *indirect_block;
        uint32_t *block_refs;
        int indirect_blocks_used = 0;

        if (inode->indirect_block == 0) {
            uint32_t indirect_block_num = find_free_block(ctx->fs_map, ctx->sb);
            if (indirect_block_num == 0) {
                result = fs_error("Erreur lors de l'allocation du bloc d'indirection");
                goto cleanup;
            }

            set_block_used(ctx->fs_map, indirect_block_num);
            printf("[DEBUG] Bloc alloué (bloc d'indirection): %u\n", indirect_block_num);
            inode->indirect_block = indirect_block_num;

            indirect_block = get_block(ctx->fs_map, (int) indirect_block_num);
            if (!indirect_block) {
                result = fs_error("Erreur lors de l'accès au bloc d'indirection");
                goto cleanup;
            }

            memset(indirect_block->data, 0, DATA_SIZE);
            indirect_block->type = BLOCK_TYPE_INDIRECT;
            block_refs = (uint32_t *) indirect_block->data;
        } else {
            indirect_block = get_block(ctx->fs_map, (int) inode->indirect_block);
            if (!indirect_block || !verify_block_sha1(indirect_block)) {
                result = fs_error("Erreur lors de l'accès au bloc d'indirection ou bloc corrompu");
                goto cleanup;
            }
            block_refs = (uint32_t *) indirect_block->data;

            if (append) {
                for (unsigned long i = 0; i < DATA_SIZE / sizeof(uint32_t); i++) {
                    if (block_refs[i] != 0) indirect_blocks_used++;
                    else break;
                }
            }
        }

        int indirect_index = append ? indirect_blocks_used : 0;
        while (remaining > 0 && (unsigned long) indirect_index < DATA_SIZE / sizeof(uint32_t)) {
            uint32_t block_num;
            if (!append || indirect_index >= indirect_blocks_used) {
                block_num = find_free_block(ctx->fs_map, ctx->sb);
                if (block_num == 0) {
                    result = fs_error("Erreur lors de l'allocation d'un bloc indirect");
                    goto cleanup;
                }

                set_block_used(ctx->fs_map, block_num);
                printf("[DEBUG] Bloc alloué (indirect): %u\n", block_num);
                block_refs[indirect_index] = block_num;
            } else {
                block_num = block_refs[indirect_index];
            }

            block_t *data_block = get_block(ctx->fs_map, (int) block_num);
            if (!data_block) {
                result = fs_error("Erreur lors de l'accès à un bloc indirect");
                goto cleanup;
            }

            if (!append || indirect_index >= indirect_blocks_used) {
                memset(data_block->data, 0, DATA_SIZE);
                data_block->type = BLOCK_TYPE_DATA;
            }

            uint32_t to_write = (remaining < DATA_SIZE) ? remaining : DATA_SIZE;
            memcpy(data_block->data, data + bytes_written, to_write);
            compute_block_sha1(data_block);

            bytes_written += to_write;
            remaining -= to_write;
            indirect_index++;
        }

        compute_block_sha1(indirect_block);

        if (remaining > 0) {
            result = fs_error("Espace insuffisant pour écrire toutes les données");
            goto cleanup;
        }
    }

    // Mettre à jour la taille de l'inode uniquement si tout s'est bien passé
    inode->size = total_size;

    cleanup:
    // Déverrouiller le mutex s'il a été verrouillé
    if (mutex_locked) {
        pthread_mutex_unlock(mutex_ptr_r);
        pthread_mutex_unlock(mutex_ptr);
    }
    compute_block_sha1(inode_block);

    return result;
}

/**
 * Crée un nouveau fichier ou réinitialise un fichier existant
 * @param ctx Contexte du système de fichiers
 * @param filename Nom du fichier à créer/réinitialiser
 * @param check_write Vérifier les permissions d'écriture si le fichier existe
 * @return Index de l'inode créé/réinitialisé ou -1 en cas d'erreur
 */
int create_or_reset_file(fs_context_t *ctx, const char *filename, int check_write) {
    int inode_index = find_inode_by_name(ctx->fs_map, filename);
    int result;
    int mutex_locked = 0;
    pthread_mutex_t *mutex_ptr = NULL;
    block_t *inode_block = NULL;
    inode_t *inode = NULL;

    if (inode_index >= 0) {
        // Cas de réinitialisation d'un fichier existant
        inode_block = get_inode_block(ctx->fs_map, inode_index);
        if (!inode_block || !verify_block_sha1(inode_block)) {
            return fs_error("Erreur lors de l'accès à l'inode ou inode corrompu");
        }

        inode = (inode_t *) inode_block->data;
        mutex_ptr = (pthread_mutex_t *) inode_block->lock_write;

        int lock_result = pthread_mutex_lock(mutex_ptr);
        if (lock_result != 0) {
            return fs_error("Erreur lors du lock");
        }

        mutex_locked = 1;

        if (!(inode->flags & PERM_EXISTS)) {
            result = fs_error("Le fichier '%s' n'existe pas", filename);
            goto cleanup;
        }

        if (check_write && !check_permissions(inode, PERM_WRITE)) {
            result = fs_error("Permission d'écriture refusée pour '%s'", filename);
            goto cleanup;
        }

        // Libérer tous les blocs directs
        for (int i = 0; i < 10; i++) {
            if (inode->direct_blocks[i] != 0) {
                set_block_free(ctx->fs_map, inode->direct_blocks[i]);
                increment_free_blocks(ctx->fs_map);
                inode->direct_blocks[i] = 0;
            }
        }

        // Libérer tous les blocs indirects et le bloc d'indirection
        if (inode->indirect_block != 0) {
            block_t *indirect_block = get_block(ctx->fs_map, (int) inode->indirect_block);
            if (indirect_block && verify_block_sha1(indirect_block)) {
                uint32_t *block_refs = (uint32_t *) indirect_block->data;
                for (unsigned long i = 0; i < DATA_SIZE / sizeof(uint32_t); i++) {
                    if (block_refs[i] != 0) {
                        set_block_free(ctx->fs_map, block_refs[i]);
                        increment_free_blocks(ctx->fs_map);
                    } else {
                        break;
                    }
                }
            }
            set_block_free(ctx->fs_map, inode->indirect_block);
            increment_free_blocks(ctx->fs_map);
            inode->indirect_block = 0;
        }

        // Réinitialiser la taille du fichier
        inode->size = 0;
        compute_block_sha1(inode_block);
        result = inode_index;
    } else {
        // Cas de création d'un nouveau fichier
        for (uint32_t i = 0; i < ctx->sb->max_inodes; i++) {
            inode_block = get_inode_block(ctx->fs_map, (int) i);
            if (!inode_block || !verify_block_sha1(inode_block)) continue;

            inode = (inode_t *) inode_block->data;
            mutex_ptr = (pthread_mutex_t *) inode_block->lock_write;

            int lock_result = pthread_mutex_lock(mutex_ptr);
            if (lock_result != 0) {
                continue;  // Essayer avec le prochain inode
            }

            mutex_locked = 1;

            if (!(inode->flags & PERM_EXISTS)) {
                // Initialiser un nouvel inode
                memset(inode, 0, sizeof(inode_t));
                strncpy(inode->filename, filename, 255);
                inode->filename[255] = '\0';
                inode->flags = PERM_EXISTS | PERM_READ | PERM_WRITE;
                inode->size = 0;

                compute_block_sha1(inode_block);
                result = (int) i;
                goto cleanup;
            }

            // Libérer le mutex et essayer avec le prochain inode
            pthread_mutex_unlock(mutex_ptr);
            mutex_locked = 0;
        }

        // Si on arrive ici, aucun inode libre n'a été trouvé
        result = fs_error("Aucun inode libre disponible");
    }

    cleanup:
    // Déverrouiller le mutex s'il a été verrouillé
    if (mutex_locked && mutex_ptr) {
        pthread_mutex_unlock(mutex_ptr);
    }

    return result;
}


/**
 * Trouve un fichier par son nom et vérifie les permissions d'accès
 * @param ctx Contexte du système de fichiers
 * @param filename Nom du fichier à rechercher
 * @param check_perm Permission à vérifier (PERM_READ, PERM_WRITE, etc.)
 * @return Index de l'inode trouvé ou -1 en cas d'erreur
 */
int find_file_with_perm_check(fs_context_t *ctx, const char *filename, uint32_t check_perm) {
    int inode_index = find_inode_by_name(ctx->fs_map, filename);
    if (inode_index < 0) {
        return fs_error("Fichier '%s' non trouvé", filename);
    }

    // Récupérer le bloc de l'inode
    block_t *inode_block = get_inode_block(ctx->fs_map, inode_index);
    if (!inode_block || !verify_block_sha1(inode_block)) {
        return fs_error("Erreur lors de l'accès à l'inode ou inode corrompu");
    }

    // Récupérer les informations de l'inode
    inode_t *inode = (inode_t *) inode_block->data;

    // Vérifier l'existence
    if (!(inode->flags & PERM_EXISTS)) {
        return fs_error("Le fichier '%s' n'existe pas", filename);
    }

    // Vérifier les permissions
    if (check_perm && !check_permissions(inode, check_perm)) {
        if (check_perm & PERM_READ) {
            return fs_error("Permission de lecture refusée pour '%s'", filename);
        }
        if (check_perm & PERM_WRITE) {
            return fs_error("Permission d'écriture refusée pour '%s'", filename);
        }
        return fs_error("Permission refusée pour '%s'", filename);
    }

    return inode_index;
}