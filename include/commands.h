//
// Created by Samuel on 16/03/2025.
//

#ifndef PSA_PROJECT_COMMANDS_H
#define PSA_PROJECT_COMMANDS_H

// Declaration des fonctions de commande
// Chaque fnct renvoie un code d'erreur (0 en cas de succès)

/**
 * Crée un nouveau système de fichiers
 * @param fsname Nom du fichier conteneur
 * @param argc Nombre d'arguments additionnels
 * @param argv Arguments additionnels
 * @return Code d'erreur
 */
int cmd_mkfs(const char *fsname, int nb_inode, int nb_block);

/**
 * Liste les fichiers dans le système de fichiers
 * @param fsname Nom du fichier conteneur
 * @param argc Nombre d'arguments additionnels
 * @param argv Arguments additionnels
 * @return Code d'erreur
 */
int cmd_ls(const char *fsname, int argc, char **argv);

/**
 * Affiche l'espace disponible dans le système de fichiers
 * @param fsname Nom du fichier conteneur
 * @param argc Nombre d'arguments additionnels
 * @param argv Arguments additionnels
 * @return Code d'erreur
 */
int cmd_df(const char *fsname);
/**
 * Copie un fichier vers ou depuis le système de fichiers
 * @param fsname Nom du fichier conteneur
 * @param source Chemin du fichier source
 * @param destination Chemin du fichier destination
 * @return Code d'erreur
 */
int cmd_cp(const char *fsname, const char *source, const char *destination);

/**
 * Supprime un fichier du système de fichiers
 * @param fsname Nom du fichier conteneur
 * @param filename Nom du fichier à supprimer
 * @return Code d'erreur
 */
int cmd_rm(const char *fsname, const char *filename);

/**
 * Verrouille un fichier en lecture ou écriture
 * @param fsname Nom du fichier conteneur
 * @param filename Nom du fichier à verrouiller
 * @param mode Mode de verrouillage (r/w)
 * @return Code d'erreur
 */
int cmd_lock(const char *fsname, const char *filename, const char *mode);

/**
 * Modifie les droits d'accès d'un fichier
 * @param fsname Nom du fichier conteneur
 * @param filename Nom du fichier à modifier
 * @param mode Nouveaux droits d'accès
 * @return Code d'erreur
 */
int cmd_chmod(const char *fsname, const char *filename, const char *mode);

/**
 * Affiche le contenu d'un fichier
 * @param fsname Nom du fichier conteneur
 * @param filename Nom du fichier à afficher
 * @return Code d'erreur
 */
int cmd_cat(const char *fsname, const char *filename);

/**
 * Écrit l'entrée standard dans un fichier
 * @param fsname Nom du fichier conteneur
 * @param filename Nom du fichier où écrire
 * @return Code d'erreur
 */
int cmd_input(const char *fsname, const char *filename);

/**
 * Concatène le contenu d'un fichier source à la fin d'un fichier destination
 * @param fsname Nom du fichier conteneur
 * @param source Nom du fichier source
 * @param destination Nom du fichier destination
 * @return Code d'erreur
 */
int cmd_add(const char *fsname, const char *source, const char *destination);

/**
 * Lit l'entrée standard et l'écrit à la suite du fichier spécifié
 * Fonctionne comme "cat - >> fichier" pour un fichier régulier
 * @param fsname Nom du fichier conteneur
 * @param filename Nom du fichier à modifier
 * @return Code d'erreur
 */
int cmd_addinput(const char *fsname, const char *filename);

/**
 * Initialise la structure de verrouillage du système de fichiers
 * @param fsname Nom du fichier conteneur
 * @return Code d'erreur
 */
int cmd_mount(const char *fsname);

/**
 * Vérifie l'intégrité du système de fichiers
 * @param fsname Nom du fichier conteneur
 * @return Code d'erreur
 */
int cmd_fsck(const char *fsname);

/**
 * Commande pour rechercher des fichiers par nom
 * @param fsname Nom du système de fichiers
 * @param pattern Motif à rechercher dans les noms de fichiers
 * @return EXIT_SUCCESS ou EXIT_FAILURE
 */
int cmd_find(const char *fsname, const char *pattern);

/**
 * Crée un nouveau répertoire dans le système de fichiers
 * @param fsname Nom du fichier conteneur
 * @param dirname Nom du répertoire à créer
 * @return Code d'erreur
 */
int cmd_mkdir(const char *fsname, const char *dirname);

/**
 * Supprime un répertoire du système de fichiers
 * @param fsname Nom du fichier conteneur
 * @param dirname Nom du répertoire à supprimer
 * @return Code d'erreur
 */
int cmd_rmdir(const char *fsname, const char *dirname);

#endif //PSA_PROJECT_COMMANDS_H