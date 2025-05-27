# Système de Fichiers Pignoufs

## Contributeur

- **Ziad ACHACH**
- **Samuel TARDIEU**

## Description

Ce projet implémente un système de fichiers en espace utilisateur stocké dans un fichier conteneur. Il offre des commandes similaires à celles d'un système de fichiers classique (`ls`, `cp`, `rm`, etc.) et assure l'intégrité des données via un mécanisme de hachage SHA1.

## Fonctionnalités principales

### Gestion des fichiers et répertoires
- Création et suppression de fichiers
- Gestion des inodes et des blocs de données
- Gestion optionnelle des sous-répertoires

### Intégrité et sécurité
- Vérification de l'intégrité via SHA1
- Gestion avancée des erreurs et des corruptions
- Outils de diagnostic (`fsck`)

### Accès et concurrence
- Verrouillage des fichiers en lecture et écriture
- Gestion de l'accès concurrentiel avec `pthread`

## Commandes principales

- `pignoufs mkfs <fsname> <nb_i> <nb_a>` : Crée un système de fichiers
- `pignoufs ls <fsname>` : Liste les fichiers
- `pignoufs cp <fsname> <src> <dest>` : Copie des fichiers
- `pignoufs rm <fsname> <file>` : Supprime un fichier
- `pignoufs cat <fsname> <file>` : Affiche un fichier
- `pignoufs fsck <fsname>` : Vérifie l'intégrité du système

## Installation

[Instructions pour compiler et exécuter le projet]

## Utilisation

[Exemples d'utilisation des commandes]

---

Ce projet est développé dans le cadre du cours de Systèmes Avancés et respecte les spécifications données dans le sujet.

