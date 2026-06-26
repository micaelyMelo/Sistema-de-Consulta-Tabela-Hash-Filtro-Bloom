#ifndef BLOOM_H
#define BLOOM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BLOOM_BITS      1000000  // m: tamanho do vetor de bits
#define BLOOM_HASHCOUNT 7        // k: número de funções hash
#define BLOOM_BYTES     ((BLOOM_BITS + 7) / 8)

typedef struct {
    uint8_t *bits;          // Ponteiro para o vetor de bits
    int      m;             // Tamanho do vetor em bits
    int      k;             // Número de funções hash
    long     count;         // Elementos inseridos
} BloomFilter;


BloomFilter *bloom_create(int m, int k);
void         bloom_destroy(BloomFilter *bf);
unsigned int bloom_hash1(const char *key); // Implementa djb2
unsigned int bloom_hash2(const char *key); //Implementa sdbm

// Operações principais do filtro 
void         bloom_insert(BloomFilter *bf, const char *key);
int          bloom_query(BloomFilter *bf, const char *key);

// Função auxiliar para exibir as estatísticas
void         bloom_print_stats(BloomFilter *bf);

// Macros para manipulação de bits
// Atribui 1 a um bit específico usando a operação OR (|). 
// Se o bit era 0, vira 1. Se já era 1, continua 1.
#define BLOOM_SET(bf, pos)   ((bf)->bits[(pos) / 8] |=  (1 << ((pos) % 8)))

// Verifica o valor de um bit específico usando a operação AND (&).
// Retorna um valor diferente de zero se o bit for 1, e zero se for 0.
#define BLOOM_GET(bf, pos)   ((bf)->bits[(pos) / 8] &   (1 << ((pos) % 8)))

#endif 