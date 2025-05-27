#!/bin/bash
set -euo pipefail

FS=testfs.img
SRC=text
OUT=output.txt

# Nettoyage
rm -f $FS $SRC $OUT append.txt
echo "Ceci est un test de Pignoufs." > $SRC

echo "Création du système de fichiers"
./../bin/pignoufs mkfs $FS 10 50
echo "Test cp (fichier vers pignoufs)"
./../bin/pignoufs cp $FS $SRC //test1.txt
./../bin/pignoufs cp $FS $SRC //test2.txt

echo "Test ls (contenu racine)"
./../bin/pignoufs ls $FS
echo "Test ls avec fichier"
./../bin/pignoufs ls $FS //test1.txt
echo "Test cat"
./../bin/pignoufs cat $FS //test1.txt > $OUT
diff $SRC $OUT && echo "cat OK"
echo "Test input (remplace le contenu)"
echo "Autre contenu" | ./../bin/pignoufs input $FS //test1.txt
./../bin/pignoufs cat $FS //test1.txt
echo "Test chmod (-w, +w)"
./../bin/pignoufs chmod $FS //test1.txt -w
./../bin/pignoufs chmod $FS //test1.txt +w
echo "Fichier de base" > base.txt
./../bin/pignoufs cp testfs.img base.txt //test1.txt     # Crée le fichier interne
echo "Ajout" > append.txt
./../bin/pignoufs add testfs.img append.txt //test1.txt  # Ajoute le contenu
./../bin/pignoufs cat testfs.img //test1.txt

echo "Test df"
./../bin/pignoufs df $FS

echo "Test lock lecture (non bloquant ici)"
timeout 1 ./../bin/pignoufs lock $FS //test1.txt r &

sleep 5
echo "Test lock écriture (devrait bloquer un moment)"
timeout 2 ./../bin/pignoufs lock $FS //test1.txt w || echo "timeout attendu (verrou actif)"

echo "Test rm"
./../bin/pignoufs rm $FS //test1.txt || echo "Erreur lors du rm"

echo "Test fsck"
./../bin/pignoufs fsck $FS

echo "Tous les tests sont terminés."
rm -f $FS $SRC $OUT append.txt base.txt

