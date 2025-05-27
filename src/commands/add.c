#include "../../include/pignoufs.h"
#include "../../include/fs_structs.h"
#include "../../include/block_ops.h"
#include "../../include/inode_ops.h"

int cmd_add(const char *fsname, const char *source, const char *destination) {
    fs_context_t ctx;
    void *src_map = NULL;
    ssize_t src_size = 0;

    if (strncmp(destination, "//", 2) == 0) {
        destination += 2;
    }

    // Initialiser le contexte Pignoufs
    if (init_fs_context_and_verify(fsname, &ctx, O_RDWR) < 0) {
        return EXIT_FAILURE;
    }

    // Ouvrir le fichier source réel
    int fd = open(source, O_RDONLY);
    if (fd < 0) {
        perror("Erreur lors de l'ouverture du fichier source");
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    // Obtenir la taille du fichier source
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Erreur lors du fstat");
        close(fd);
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }
    src_size = st.st_size;

    // Mapper le fichier source en mémoire
    src_map = mmap(NULL, src_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (src_map == MAP_FAILED) {
        perror("Erreur mmap source");
        close(fd);
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    close(fd); // on peut fermer après mmap

    // Trouver l'inode destination (dans le FS Pignoufs)
    int dest_inode_index = find_file_with_perm_check(&ctx, destination, PERM_WRITE);
    if (dest_inode_index < 0) {
        fs_error("Fichier destination introuvable ou non accessible en écriture\n");
        munmap(src_map, (int) src_size);
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    // Écrire à la fin du fichier interne (append = 1)
    if (write_inode_content(&ctx, dest_inode_index, src_map, src_size, 1) < 0) {
        fs_error("Erreur lors de l'ajout au fichier destination\n");
        munmap(src_map, (int) src_size);
        fs_free_context(&ctx);
        return EXIT_FAILURE;
    }

    printf("Contenu de '%s' ajouté avec succès à '%s' (%zu octets)\n",
           source, destination, src_size);

    munmap(src_map, (int) src_size);
    fs_free_context(&ctx);
    return EXIT_SUCCESS;
}
