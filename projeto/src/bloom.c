#include "bloom.h"
#include <math.h>

// bloom_create - Aloca e inicializa o filtro de Bloom
// m: tamanho em bits; k: número de funções hash 
BloomFilter *bloom_create(int m, int k) {
    // Aloca memória para a estrutura (cabeçalho) do Filtro de Bloom
    BloomFilter *bf = (BloomFilter *)malloc(sizeof(BloomFilter));
    if (!bf) return NULL; // Proteção contra falha de alocação


    // Como cada byte tem 8 bits, (m + 7) / 8 garante que teremos
    // bytes suficientes mesmo que 'm' não seja múltiplo exato de 8.
    int bytes = (m + 7) / 8;


    // O calloc é usado no lugar do malloc porque ele já inicializa
    // todos os bytes com zero (0x00), ou seja, todos os bits começam em 0.
    bf->bits = (uint8_t *)calloc(bytes, 1);
    if (!bf->bits) { free(bf); return NULL; } // Libera a struct se o vetor falhar

    // Armazena as configurações no filtro
    bf->m     = m;
    bf->k     = k;
    bf->count = 0;
    return bf;
}

// bloom_destroy - Libera memória do filtro
void bloom_destroy(BloomFilter *bf) {
    if (!bf) return; // Evita falha de segmentação ao tentar liberar ponteiro nulo
    free(bf->bits);  // Libera o array de bits primeiro
    free(bf);        // Libera a estrutura principal em seguida
}

// bloom_hash1 - djb2: boa distribuição para strings
unsigned int bloom_hash1(const char *key) {
    unsigned long hash = 5381;
    int c;

    // Percorre a string até encontrar o caractere nulo ('\0')
    while ((c = (unsigned char)*key++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return (unsigned int)hash;
}

// bloom_hash2 - sdbm: complementar ao djb2
unsigned int bloom_hash2(const char *key) {
    unsigned long hash = 0;
    int c;

    // Percorre a string caractere por caractere
    while ((c = (unsigned char)*key++)) {
        hash = c + (hash << 6) + (hash << 16) - hash; 
    }
    return (unsigned int)hash;
}

// bloom_insert - Marca k posições no vetor de bits
// gi(x) = (h1 + i * h2) mod m
void bloom_insert(BloomFilter *bf, const char *key) {

    // Calcula os dois hashes base apenas UMA vez para economizar processamento
    unsigned int h1 = bloom_hash1(key);
    unsigned int h2 = bloom_hash2(key);

    // Gera os 'k' índices virtuais combinando h1 e h2
    for (int i = 0; i < bf->k; i++) {
        unsigned int pos = (h1 + (unsigned int)i * h2) % (unsigned int)bf->m;
        BLOOM_SET(bf, pos);
    }
    bf->count++;  
}

// bloom_query - Verifica se todos os k bits estão marcados
// Retorna: 1 = "possivelmente existe"
//          0 = "definitivamente não existe"
int bloom_query(BloomFilter *bf, const char *key) {
    unsigned int h1 = bloom_hash1(key);
    unsigned int h2 = bloom_hash2(key);

    for (int i = 0; i < bf->k; i++) {
        unsigned int pos = (h1 + (unsigned int)i * h2) % (unsigned int)bf->m;
        if (!BLOOM_GET(bf, pos)) return 0;  // Definitivamente ausente
    }
    return 1;  // Possivelmente presente
}

// bloom_print_stats - Exibe estatísticas do filtro
void bloom_print_stats(BloomFilter *bf) {
    if (!bf) return;

    // Conta bits setados
    long bits_set = 0;
    int bytes = (bf->m + 7) / 8;
    for (int i = 0; i < bytes; i++) {
        uint8_t byte = bf->bits[i];
        while (byte) { bits_set += (byte & 1); byte >>= 1; }
    }

    // Calcula a porcentagem do filtro que já está ocupada
    double fill_rate = (double)bits_set / bf->m * 100.0;

    // Taxa teórica de falsos positivos: (1 - e^(-k*n/m))^k 
    // Essa é a fórmula matemática clássica para prever a colisão no Filtro de Bloom.
    double n = (double)bf->count;
    double m = (double)bf->m;
    double k = (double)bf->k;
    double fp_theory = pow(1.0 - exp(-k * n / m), k) * 100.0;

    // Imprime o relatório de análise
    printf("  Vetor de bits       : %d bits (%d bytes, ~%.1f KB)\n",
           bf->m, bytes, (double)bytes / 1024.0);
    printf("  Funcoes hash (k)    : %d\n", bf->k);
    printf("  Elementos inseridos : %ld\n", bf->count);
    printf("  Bits marcados       : %ld / %d (%.2f%%)\n",
           bits_set, bf->m, fill_rate);
    printf("  FP teorico (formula): %.4f%%\n", fp_theory);
}