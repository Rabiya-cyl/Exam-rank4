// ==============================================================================
// INCLUSIONS DES BIBLIOTHÈQUES STANDARDS
// ==============================================================================
#include <stdio.h>      // Fonctions d'entrée/sortie (printf, fopen, getc, etc.)
#include <stdbool.h>    // Pour utiliser le type `bool` (true, false)
#include <ctype.h>      // Fonctions de test de caractères (isdigit, etc.)
#include <string.h>     // Fonctions sur les chaînes de caractères (strdup, etc.)
#include <stdlib.h>     // Fonctions utilitaires (gestion mémoire : malloc, realloc, free)


// ==============================================================================
// DÉFINITION DES STRUCTURES DE DONNÉES
// ==============================================================================

// Structure `json` : représente un élément JSON (objet, entier, ou chaîne)
typedef struct  json {
    // Énumération pour identifier le type de donnée stocké dans l'union
    enum {
        MAP,        // Représente un objet JSON, ex: { "cle": "valeur" }
        INTEGER,    // Représente un nombre entier
        STRING      // Représente une chaîne de caractères
    } type;
    // Union : un seul de ses membres est actif à la fois, économisant la mémoire.
    // Le membre `type` ci-dessus nous dit lequel est actif.
    union {
        // Si type == MAP
        struct {
            struct pair *data; // Tableau dynamique de paires clé/valeur
            size_t      size;      // Nombre d'éléments dans le tableau `data`
        } map;
        // Si type == INTEGER
        int integer;
        // Si type == STRING
        char    *string;
    };
}   json;

// Structure `pair` : représente une paire clé/valeur dans un objet JSON (MAP)
typedef struct  pair {
    char    *key;    // La clé (toujours une chaîne de caractères)
    json    value;   // La valeur (qui est elle-même un objet json)
}   pair;


// ==============================================================================
// PROTOTYPES DES FONCTIONS
// ==============================================================================
int     argo(json *dst, FILE *stream);
int     parser(json *dst, FILE *stream);
int     parse_int(json *dst, FILE *stream);
int     parse_string(json *dst, FILE *stream);
int     parse_map(json *dst, FILE *stream);
void    free_json(json j);


// ==============================================================================
// FONCTIONS UTILITAIRES DE PARSING
// ==============================================================================

// `peek` : lit le prochain caractère du flux sans le retirer (le "consommer")
int peek(FILE *stream)
{
    int c = getc(stream); // Lit le caractère
    ungetc(c, stream);    // Le remet immédiatement dans le flux
    return c;             // Retourne le caractère lu
}

// `unexpected` : Affiche un message d'erreur pour un caractère inattendu
void    unexpected(FILE *stream)
{
    if (peek(stream) != EOF) // Si ce n'est pas la fin du fichier
        printf("unexpected token '%c'\n", peek(stream));
    else // Si c'est la fin du fichier
        printf("unexpected end of input\n");
}

// `accept` : Si le prochain caractère est `c`, le consomme et retourne 1 (vrai). Sinon retourne 0.
int accept(FILE *stream, char c)
{
    if (peek(stream) == c)
    {
        (void)getc(stream); // Consomme le caractère
        return 1;
    }
    return 0;
}

// `expect` : Exige que le prochain caractère soit `c`. Si ce n'est pas le cas, affiche une erreur.
int expect(FILE *stream, char c)
{
    if (accept(stream, c)) // Tente de consommer le caractère
        return 1;
    unexpected(stream); // Sinon, erreur
    return 0;
}


// ==============================================================================
// GESTION MÉMOIRE ET SÉRIALISATION
// ==============================================================================

// `free_json` : Libère récursivement la mémoire allouée pour une structure json
void    free_json(json j)
{
    switch (j.type)
    {
        case MAP: // Si c'est un objet
            // Pour chaque paire clé/valeur...
            for (size_t i = 0; i < j.map.size; i++)
            {
                free(j.map.data[i].key);      // Libère la mémoire de la clé (chaîne)
                free_json(j.map.data[i].value); // Appel récursif pour libérer la valeur
            }
            free(j.map.data); // Libère le tableau de paires
            break ;
        case STRING: // Si c'est une chaîne
            free(j.string); // Libère la mémoire de la chaîne
            break ;
        default: // Pour INTEGER, rien à faire car pas d'allocation dynamique
            break ;
    }
}

// `serialize` : Affiche une structure json sous forme de chaîne de caractères (le contraire du parsing)
void    serialize(json j)
{
    switch (j.type)
    {
        case INTEGER:
            printf("%d", j.integer);
            break ;
        case STRING:
            putchar('"');
            for (int i = 0; j.string[i]; i++)
            {
                // Échappe les caractères spéciaux `\` et `"`
                if (j.string[i] == '\\' || j.string[i] == '"')
                    putchar('\\');
                putchar(j.string[i]);
            }
            putchar('"');
            break ;
        case MAP:
            putchar('{');
            for (size_t i = 0; i < j.map.size; i++)
            {
                if (i != 0) // Ajoute une virgule avant chaque élément sauf le premier
                    putchar(',');
                // Affiche la clé (qui est une chaîne)
                serialize((json){.type = STRING, .string = j.map.data[i].key});
                putchar(':');
                // Appel récursif pour afficher la valeur
                serialize(j.map.data[i].value);
            }
            putchar('}');
            break ;
    }
}


// ==============================================================================
// CŒUR DU PARSER (ANALYSE SYNTAXIQUE)
// ==============================================================================

// `parser` : Fonction principale de parsing, qui redirige vers la fonction spécialisée
int parser(json *dst, FILE *stream)
{
    int c = peek(stream); // Regarde le premier caractère
    if (c == '"')
        return (parse_string(dst, stream)); // C'est une chaîne
    else if (isdigit(c) || c == '-')
        return (parse_int(dst, stream));    // C'est un entier
    else if (c == '{')
        return (parse_map(dst, stream));    // C'est un objet (map)
    else
    {
        unexpected(stream); // Caractère inconnu, erreur
        return (-1);
    }
}

// `parse_int` : Analyse un entier
int parse_int(json *dst, FILE *stream)
{
    int n;
    // Tente de lire un entier depuis le flux
    if (fscanf(stream, "%d", &n) == 1)
    {
        dst->type = INTEGER; // Définit le type
        dst->integer = n;    // Stocke la valeur
        return (1); // Succès
    }
    unexpected(stream); // Échec de la lecture
    return (-1);
}

// `parse_string` : Analyse une chaîne de caractères
int parse_string(json *dst, FILE *stream)
{
    char    buffer[4096]; // Tampon pour stocker la chaîne lue
    char    c;
    int     i;

    if (!expect(stream, '"')) // Doit commencer par un guillemet
        return (-1);
    i = 0;
    while (1)
    {
        c = getc(stream); // Lit le caractère suivant
        if (c == EOF)
        {
            unexpected(stream); // Fin de fichier inattendue
            return (-1);
        }
        if (c == '"') // Guillemet de fin
            break ;
        if (c == '\\') // Caractère d'échappement
        {
            c = getc(stream); // Lit le caractère suivant (celui qui est échappé)
            if (c == EOF)
            {
                unexpected(stream);
                return (-1);
            }
        }
        buffer[i++] = c; // Ajoute le caractère au tampon
    }
    buffer[i] = '\0'; // Termine la chaîne avec le caractère nul
    dst->type = STRING;
    dst->string = strdup(buffer); // Alloue de la mémoire et copie la chaîne
    return (1); // Succès
}

// `parse_map` : Analyse un objet JSON
int parse_map(json *dst, FILE *stream)
{
    pair    *items;
    size_t  size;
    json    key; // Variable temporaire pour stocker la clé lue

    if (!expect(stream, '{')) // Doit commencer par une accolade ouvrante
        return (-1);

    items = NULL; // Initialise le tableau de paires
    size = 0;     // Initialise la taille

    // Boucle tant qu'on ne trouve pas l'accolade fermante
    while (!accept(stream, '}'))
    {
        // Agrandit le tableau pour accueillir une nouvelle paire
        items = realloc(items, sizeof(pair) * (size + 1));
        
        // 1. Analyse de la clé (doit être une chaîne)
        if (parse_string(&key, stream) == -1)
        {
            free(items);
            return (-1);
        }
        // 2. Vérifie la présence des deux-points
        if (!expect(stream, ':'))
        {
            free(key.string);
            free(items);
            return (-1);
        }
        // 3. Analyse de la valeur (appel récursif au parser principal)
        if (parser(&items[size].value, stream) == -1)
        {
            free(key.string);
            free(items);
            return (-1);
        }
        items[size].key = key.string; // Assigne la clé
        size++; // Incrémente la taille
        
        // 4. Vérifie s'il y a une virgule (pour une autre paire) ou une accolade (fin de l'objet)
        if (!accept(stream, ',') && peek(stream) != '}')
        {
            unexpected(stream);
            free(items); // Nettoyage en cas d'erreur
            return (-1);
        }
    }
    dst->type = MAP;
    dst->map.size = size;
    dst->map.data = items;
    return (1); // Succès
}

// `argo` : Point d'entrée pour le parsing, simple wrapper pour `parser`
int argo(json *dst, FILE *stream)
{
	// Initialise la structure à un état sûr par défaut.
    // Si une erreur se produit plus tard, `main` appellera `free_json`
    // sur un type INTEGER, ce qui est sans danger.
	dst->type = INTEGER;
    dst->integer = 0;
	//fin de l'ajout
    return (parser(dst, stream));
}

// ==============================================================================
// FONCTION PRINCIPALE (MAIN)
// ==============================================================================
int main(int argc, char **argv)
{
    // Vérifie que le programme est appelé avec un nom de fichier en argument
    if (argc != 2)
        return 1; // Erreur d'utilisation
    
    char *filename = argv[1];
    FILE *stream = fopen(filename, "r"); // Ouvre le fichier en lecture

    json    file; // Variable qui contiendra la structure JSON parsée
    
    // Lance le parsing
    if (argo (&file, stream) != 1)
    {
        // En cas d'erreur, libère ce qui a pu être alloué
        free_json(file);
        return 1; // Quitte avec un code d'erreur
    }

    // Si le parsing a réussi, affiche le résultat formaté
    serialize(file);
    printf("\n"); // Pour une sortie propre
    
    // Libère toute la mémoire allouée pendant le parsing
    free_json(file);
    return 0; // Succès
}