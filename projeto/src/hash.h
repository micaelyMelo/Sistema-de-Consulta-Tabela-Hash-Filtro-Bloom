#ifndef HASH_H
#define HASH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_TABLE_SIZE 131101 
#define MAX_KEY_LEN     64

// Nó da lista encadeada para tratamento de colisão
typedef struct HashNode {
    char key[MAX_KEY_LEN];   // Identificador do usuário
    struct HashNode *next;   // Próximo nó na cadeia
} HashNode;

// Estrutura da Tabela Hash
typedef struct {
    HashNode **buckets;      // Vetor de ponteiros para listas
    int size;                // Número de buckets
    int count;               // Número de elementos inseridos
    int collisions;          // Contador de colisões
} HashTable;


HashTable *hash_create(int size);                               // Aloca e inicia a tabela
void       hash_destroy(HashTable *ht);                         // Libera toda a memória da tabela
unsigned int hash_function(HashTable *ht, const char *key);     // Algoritmo de espalhamento djb2
int        hash_insert(HashTable *ht, const char *key);         // Insere nova chave no início
int        hash_search(HashTable *ht, const char *key);         // Busca por uma chave na tabela
void       hash_print_stats(HashTable *ht);                     // Imprime estatísticas de desempenho

#endif 