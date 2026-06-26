#include "hash.h"

// hash_create - Aloca e inicializa a tabela hash
// size: número de buckets
HashTable *hash_create(int size) {
    // Aloca a estrutura principal da tabela
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    if (!ht) return NULL;

    // calloc zera a memória, garantindo que os ponteiros iniciem como NULL
    ht->buckets = (HashNode **)calloc(size, sizeof(HashNode *));
    if (!ht->buckets) { free(ht); return NULL; }

    ht->size       = size;
    ht->count      = 0;
    ht->collisions = 0;
    return ht;
}

// hash_destroy - Libera toda a memória da tabela
void hash_destroy(HashTable *ht) {
    if (!ht) return;

    // Percorre cada bucket da tabela
    for (int i = 0; i < ht->size; i++) {
        HashNode *cur = ht->buckets[i];
        
        // Libera todos os nós da lista encadeada
        while (cur) {
            HashNode *tmp = cur;
            cur = cur->next;
            free(tmp);
        }
    }
    free(ht->buckets);
    free(ht);
}

// hash_function - djb2: mapeia string para índice [0, size)
unsigned int hash_function(HashTable *ht, const char *key) {
    unsigned long hash = 5381;   // Semente padrão djb2 
    int c;

    // Percorre a string caractere por caractere
    while ((c = (unsigned char)*key++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    return (unsigned int)(hash % (unsigned long)ht->size);
}

// hash_insert - Insere chave na tabela
// Retorna 1 se inserido, 0 se foi duplicata
int hash_insert(HashTable *ht, const char *key) {
    unsigned int idx = hash_function(ht, key);

    // Verifica duplicata e conta colisões
    HashNode *cur = ht->buckets[idx];
    if (cur != NULL) {
        ht->collisions++;
    }
    while (cur) {
        if (strcmp(cur->key, key) == 0) return 0; 
        cur = cur->next;
    }

    // Aloca novo nó 
    HashNode *node = (HashNode *)malloc(sizeof(HashNode));
    if (!node) return -1;
    strncpy(node->key, key, MAX_KEY_LEN - 1);
    node->key[MAX_KEY_LEN - 1] = '\0';
    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;
    ht->count++;
    return 1;
}

// hash_search - Busca chave na tabela
// Retorna 1 se encontrada, 0 caso contrário
int hash_search(HashTable *ht, const char *key) {
    unsigned int idx = hash_function(ht, key);
    HashNode *cur = ht->buckets[idx];
    while (cur) {
        if (strcmp(cur->key, key) == 0) return 1;  // encontrado
        cur = cur->next;
    }
    return 0;  // não encontrado
}

// hash_print_stats - Exibe estatísticas internas da tabela hash
void hash_print_stats(HashTable *ht) {
    if (!ht) return;

    int buckets_used = 0;
    int max_chain    = 0;

    // Analisa a ocupação de cada bucket
    for (int i = 0; i < ht->size; i++) {
        if (ht->buckets[i]) {
            buckets_used++;
            int chain_len = 0;
            HashNode *cur = ht->buckets[i];
            while (cur) { chain_len++; cur = cur->next; }
            if (chain_len > max_chain) max_chain = chain_len;
        }
    }

    // Calcula a média e o fator de carga
    double load_factor = (double)ht->count / ht->size;

    printf("  Tamanho da tabela   : %d buckets\n", ht->size);
    printf("  Elementos inseridos : %d\n", ht->count);
    printf("  Buckets utilizados  : %d\n", buckets_used);
    printf("  Fator de carga      : %.4f\n", load_factor);
    printf("  Colisoes detectadas : %d\n", ht->collisions);
    printf("  Maior cadeia        : %d nos\n", max_chain);
}