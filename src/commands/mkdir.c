//
// Created by Samuel on 05/05/2025.
//

#include "../../include/pignoufs.h"
#include "../../include/fs_structs.h"
#include "../../include/dir_ops.h"

int cmd_mkdir(const char *fsname, const char *dirname) {
    fs_context_t ctx;
    int result;

    // Initialise le contexte
    result = init_fs_context_and_verify(fsname, &ctx, O_RDWR);
    if (result != FS_SUCCESS) {
        return result;
    }

    // Crée le répertoire
    result = create_directory(&ctx, dirname);

    // Libère le contexte
    fs_free_context(&ctx);

    if (result < 0) {
        // Gestion des erreurs spécifiques
        switch (result) {
            case FS_ERROR_NOTFOUND:
                return fs_error("mkdir: Le répertoire '%s' existe déjà ou un fichier avec ce nom existe", dirname);
            case FS_ERROR_FULL:
                return fs_error("mkdir: Le système de fichiers est plein");
            default:
                return fs_error("mkdir: Erreur lors de la création du répertoire '%s'", dirname);
        }
    }

    return FS_SUCCESS;
}