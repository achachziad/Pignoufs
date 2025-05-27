//
// Created by Samuel on 05/05/2025.
//

#include "../../include/pignoufs.h"
#include "../../include/fs_structs.h"
#include "../../include/dir_ops.h"

int cmd_rmdir(const char *fsname, const char *dirname) {
    fs_context_t ctx;
    int result;

    // Initialise le contexte
    result = init_fs_context_and_verify(fsname, &ctx, O_RDWR);
    if (result != FS_SUCCESS) {
        return result;
    }

    // Supprime le répertoire
    result = remove_directory(&ctx, dirname);

    // Libère le contexte
    fs_free_context(&ctx);

    if (result != FS_SUCCESS) {
        // Gestion des erreurs spécifiques
        if (result == FS_ERROR_NOTFOUND) {
            return fs_error("rmdir: Le répertoire '%s' n'existe pas ou n'est pas un répertoire", dirname);
        } else {
            return fs_error("rmdir: Erreur lors de la suppression du répertoire '%s'", dirname);
        }
    }

    return FS_SUCCESS;
}