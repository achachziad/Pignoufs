//
// Created by Samuel on 16/03/2025.
//

#include "../include/pignoufs.h"
#include "../include/commands.h"
#include "../include/fs_common.h"

// Structure représentant une commande
typedef struct {
    const char *name;

    int (*func)(const char *, int, char **);

    int min_args;
    const char *usage;
    const char *description;
} Command;


int wrapper_mkfs(const char *fsname, int argc, char **argv) {
    if (argc < 2) {
        return fs_error("Usage: mkfs <fsname> <nombre inode> <nombre blocks>");
    }
    return cmd_mkfs(fsname, atoi(argv[0]), atoi(argv[1]));
}

int wrapper_df(const char *fsname, int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);
    return cmd_df(fsname);
}

// Fonctions wrapper pour standardiser les signatures
int wrapper_cp(const char *fsname, int argc, char **argv) {
    if (argc < 2) {
        return fs_error("Usage: cp <fsname> <source> <destination>");
    }
    return cmd_cp(fsname, argv[0], argv[1]);
}

int wrapper_rm(const char *fsname, int argc, char **argv) {
    if (argc < 1) {
        return fs_error("Usage: rm <fsname> <fichier>");
    }
    return cmd_rm(fsname, argv[0]);
}

int wrapper_lock(const char *fsname, int argc, char **argv) {
    if (argc < 2) {
        fs_error("Usage: lock <fsname> <fichier> <mode>\n");
        return EXIT_FAILURE;
    }
    return cmd_lock(fsname, argv[0], argv[1]);
}

int wrapper_chmod(const char *fsname, int argc, char **argv) {
    if (argc < 2) {
        fs_error("Usage: chmod <fsname> <fichier> <mode>\n");
        return EXIT_FAILURE;
    }
    return cmd_chmod(fsname, argv[0], argv[1]);
}

int wrapper_cat(const char *fsname, int argc, char **argv) {
    if (argc < 1) {
        fs_error("Usage: cat <fsname> <fichier>\n");
        return EXIT_FAILURE;
    }
    return cmd_cat(fsname, argv[0]);
}

int wrapper_input(const char *fsname, int argc, char **argv) {
    if (argc < 1) {
        fs_error("Usage: input <fsname> <fichier>\n");
        return EXIT_FAILURE;
    }
    return cmd_input(fsname, argv[0]);
}

int wrapper_add(const char *fsname, int argc, char **argv) {
    if (argc < 2) {
        fs_error("Usage: add <fsname> <source> <destination>\n");
        return EXIT_FAILURE;
    }
    return cmd_add(fsname, argv[0], argv[1]);
}

int wrapper_addinput(const char *fsname, int argc, char **argv) {
    if (argc < 1) {
        fs_error("Usage: addinput <fsname> <fichier>\n");
        return EXIT_FAILURE;
    }
    return cmd_addinput(fsname, argv[0]);
}

int wrapper_find(const char *fsname, int argc, char **argv) {
    if (argc < 1) {
        fs_error("Usage: find <fsname> <fichier>\n");
        return EXIT_FAILURE;
    }
    return cmd_find(fsname, argv[0]);
}

int wrapper_mkdir(const char *fsname, int argc, char **argv) {
    if (argc < 1) {
        fs_error("Usage: mkdir <fsname> <dossier>\n");
        return EXIT_FAILURE;
    }
    return cmd_mkdir(fsname, argv[0]);
}

int wrapper_rmdir(const char *fsname, int argc, char **argv) {
    if (argc < 1) {
        fs_error("Usage: rmdir <fsname> <dossier>\n");
        return EXIT_FAILURE;
    }
    return cmd_rmdir(fsname, argv[0]);
}

int wrapper_fsck(const char *fsname, int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);
    return cmd_fsck(fsname);
}

int wrapper_mount(const char *fsname, int argc, char **argv) {
    UNUSED(argc);
    UNUSED(argv);
    return cmd_mount(fsname);
}

// Table des commandes supportées
static const Command commands[] = {
        {"mkfs",     wrapper_mkfs,     2, "mkfs <fsname> <nombre inode> <nombre blocks>", "Créer un système de fichiers"},
        {"ls",       cmd_ls,           0, "ls <fsname>",                                  "Lister les fichiers du système"},
        {"df",       wrapper_df,       0, "df <fsname>",                                  "Afficher l'espace libre"},
        {"cp",       wrapper_cp,       2, "cp <fsname> <source> <destination>",           "Copier un fichier"},
        {"rm",       wrapper_rm,       1, "rm <fsname> <fichier>",                        "Supprimer un fichier"},
        {"lock",     wrapper_lock,     2, "lock <fsname> <fichier> <mode>",               "Verrouiller un fichier (mode: read/write)"},
        {"chmod",    wrapper_chmod,    2, "chmod <fsname> <fichier> <mode>",              "Modifier les droits d'accès"},
        {"cat",      wrapper_cat,      1, "cat <fsname> <fichier>",                       "Afficher le contenu d'un fichier"},
        {"input",    wrapper_input,    1, "input <fsname> <fichier>",                     "Écrire l'entrée standard dans un fichier"},
        {"add",      wrapper_add,      2, "add <fsname> <source> <destination>",          "Ajouter un fichier à un autre"},
        {"addinput", wrapper_addinput, 1, "addinput <fsname> <fichier>",                  "Ajouter l'entrée standard à un fichier existant"},
        {"find",     wrapper_find,     1, "find <fsname> <fichier>",                      "Rechercher un fichier"},
        {"mkdir",    wrapper_mkdir,    1, "mkdir <fsname> <dossier>",                     "Créer un dossier"},
        {"rmdir",    wrapper_rmdir,    1, "rmdir <fsname> <dossier>",                     "Supprimer un dossier"},
        {"fsck",     wrapper_fsck,     0, "fsck <fsname>",                                "Vérifier l'intégrité du système de fichiers"},
        {"mount",    wrapper_mount,    0, "mount <fsname>",                               "Initialiser la structure de verrouillage"},
        {NULL, NULL,                   0, NULL, NULL} // Fin de la table
};

/**
 * Affiche l'aide avec toutes les commandes disponibles
 */
void print_usage(const char *prog_name) {
    fs_error("Usage: %s <commande> <fsname> [options]\n", prog_name);
    fs_error("Commandes disponibles:\n");

    for (const Command *cmd = commands; cmd->name != NULL; cmd++) {
        fs_error("  %-10s %-35s %s\n", cmd->name, cmd->usage, cmd->description);
    }
}

/**
 * Affiche l'usage d'une commande spécifique
 */
void print_command_usage(const char *prog_name, const Command *cmd) {
    fs_error("Usage: %s %s\n", prog_name, cmd->usage);
    fs_error("Description: %s\n", cmd->description);
}

int main(int argc, char *argv[]) {
    // Vérification des arguments minimum
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *command_name = argv[1];

    // Cas spécial pour afficher l'aide
    if (strcmp(command_name, "--help") == 0 || strcmp(command_name, "-h") == 0) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    // Vérification du nom du système de fichiers
    if (argc < 3) {
        fs_error("Erreur: nom du système de fichiers manquant.");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *fsname = argv[2];

    // Recherche de la commande dans la table
    const Command *selected_command = NULL;
    for (const Command *cmd = commands; cmd->name != NULL; cmd++) {
        if (strcmp(command_name, cmd->name) == 0) {
            selected_command = cmd;
            break;
        }
    }

    // Vérification si la commande existe
    if (selected_command == NULL) {
        fs_error("Erreur: commande inconnue: '%s'", command_name);
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Vérification du nombre minimum d'arguments
    if (argc - 3 < selected_command->min_args) {
        fs_error("Erreur: arguments insuffisants pour la commande '%s'", command_name);
        print_command_usage(argv[0], selected_command);
        return EXIT_FAILURE;
    }

    // Exécution de la commande
    return selected_command->func(fsname, argc - 3, &argv[3]);
}