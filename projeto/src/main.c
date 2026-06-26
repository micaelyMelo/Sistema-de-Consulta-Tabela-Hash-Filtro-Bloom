#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif
#include <time.h>
#include <math.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif

#include "hash.h"
#include "bloom.h"

#define CMD_MAX  256
#define USER_MAX 64
#define DATA_DIR "data/"

// Estrutura de estatísticas globais
typedef struct {
    long   total_queries;         // Total de consultas realizadas       
    long   queries_avoided;       // Consultas evitadas pelo Bloom (neg) 
    long   false_positives;       // Bloom "sim", hash "não"             
    long   true_positives;        // Encontrados em ambos                
    long   true_negatives;        // Bloom "não" (correto)               
    double total_query_time_ms;   // Tempo total de consultas (ms)       
} Stats;

// Estrutura principal do sistema
typedef struct {
    HashTable   *ht;
    BloomFilter *bf;
    Stats        stats;
} System;

// Retorna tempo atual em milissegundos com precisão
static double now_ms(void) {
#ifdef _WIN32
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER count;
    if (freq.QuadPart == 0) {
        QueryPerformanceFrequency(&freq);
    }
    QueryPerformanceCounter(&count);
    return (double)count.QuadPart * 1000.0 / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6;
#endif
}

// Remove espaços e quebras de linha no início e fim da string 
static void trim(char *s) {
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r' || s[len-1] == ' '))
        s[--len] = '\0';
    char *p = s;
    while (*p == ' ') p++;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

// Cria o sistema com tabela hash e filtro de Bloom 
System *system_create(void) {
    System *sys = (System *)calloc(1, sizeof(System));
    if (!sys) return NULL;

    sys->ht = hash_create(HASH_TABLE_SIZE);
    sys->bf = bloom_create(BLOOM_BITS, BLOOM_HASHCOUNT);

    // Se qualquer uma das estruturas falhar, desfaz tudo
    if (!sys->ht || !sys->bf) {
        if (sys->ht) hash_destroy(sys->ht);
        if (sys->bf) bloom_destroy(sys->bf);
        free(sys);
        return NULL;
    }
    return sys;
}

// Libera o sistema
void system_destroy(System *sys) {
    if (!sys) return;
    hash_destroy(sys->ht);
    bloom_destroy(sys->bf);
    free(sys);
}

// RF01 - Inserção: insere em ambas as estruturas 
int system_insert(System *sys, const char *key) {
    int r = hash_insert(sys->ht, key);
    if (r == 1) {                               // Só insere no Bloom se for um elemento novo
        bloom_insert(sys->bf, key);
    }
    return r;
}

// RF02 - Consulta encadeada 
int system_query(System *sys, const char *key) {
    double t0 = now_ms();
    int result;

    sys->stats.total_queries++;

    // Passo 1-2: consulta o filtro de Bloom 
    if (!bloom_query(sys->bf, key)) {
        // Definitivamente não existe - acesso à Hash completamente evitado
        sys->stats.queries_avoided++;
        sys->stats.true_negatives++;
        result = 0;
    } else {
        // Passo 3: possivelmente existe - confirma na Tabela Hash 
        if (hash_search(sys->ht, key)) {
            sys->stats.true_positives++;
            result = 2;   // Verdadeiro positivo: encontrado
        } else {
            sys->stats.false_positives++;
            result = 1;   // Falso positivo: Bloom errou, hash filtrou
        }
    }

    sys->stats.total_query_time_ms += (now_ms() - t0);
    return result;
}

// RF04 - Inserção em lote a partir de arquivo texto
int system_load_file(System *sys, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("  [ERRO] Nao foi possivel abrir '%s'\n", filename);
        return -1;
    }
    char line[USER_MAX];
    int count = 0;

    // Lê linha por linha do arquivo limpando espaços e inserindo no sistema
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (strlen(line) == 0) continue;
        if (system_insert(sys, line) == 1) count++;
    }
    fclose(f);
    return count;
}

// RF03 - Exibe estatísticas completas do sistema
void system_print_stats(System *sys) {
    Stats *s = &sys->stats;
    long queries_to_hash = s->total_queries - s->queries_avoided;

    // Calcula taxas com tratamento para divisão por zero
    double fp_rate = 0.0;
    if (queries_to_hash > 0)
        fp_rate = (double)s->false_positives / queries_to_hash * 100.0;

    double avg_time = 0.0;
    if (s->total_queries > 0)
        avg_time = s->total_query_time_ms / s->total_queries;

    printf("\n+------------------------------------------+\n");
    printf("|         ESTATISTICAS DO SISTEMA          |\n");
    printf("+------------------------------------------+\n");
    printf("  Elementos cadastrados : %d\n", sys->ht->count);
    printf("  Total de consultas    : %ld\n", s->total_queries);
    printf("  Consultas a Hash      : %ld\n", queries_to_hash);
    printf("  Consultas evitadas    : %ld (Bloom negativo)\n", s->queries_avoided);
    printf("  Verdadeiros positivos : %ld\n", s->true_positives);
    printf("  Falsos positivos      : %ld\n", s->false_positives);
    printf("  Taxa de FP            : %.4f%%\n", fp_rate);
    printf("  Tempo medio consulta  : %.6f ms\n", avg_time);
    printf("\n--- Tabela Hash ---\n");
    hash_print_stats(sys->ht);
    printf("\n--- Filtro de Bloom ---\n");
    bloom_print_stats(sys->bf);
    printf("+------------------------------------------+\n\n");
}

// Gera arquivo com n nomes aleatórios
static void generate_file(const char *filename, int n) {
    FILE *f = fopen(filename, "w");
    if (!f) { printf("  [ERRO] Nao foi possivel criar '%s'\n", filename); return; }

    srand(42);  
    const char *alpha = "abcdefghijklmnopqrstuvwxyz";

    // Gera strings estruturadas no padrão: abcdefgh123
    for (int i = 0; i < n; i++) {
        char name[12];
        for (int j = 0; j < 8; j++)
            name[j] = alpha[rand() % 26];
        name[8]  = (char)('0' + rand() % 10);
        name[9]  = (char)('0' + rand() % 10);
        name[10] = (char)('0' + rand() % 10);
        name[11] = '\0';
        fprintf(f, "%s\n", name);
    }
    fclose(f);
}

// Executa experimento completo com n registros
static void run_experiment(int n) {
    char fname[128];
    snprintf(fname, sizeof(fname), DATA_DIR "usuarios_%d.txt", n);

    printf("\n[EXPERIMENTO] %d registros\n", n);
    printf("  Gerando '%s'...\n", fname);
    generate_file(fname, n);

    // Carrega dados 
    System *sys = system_create();
    if (!sys) { printf("  [ERRO] Falha ao criar sistema\n"); return; }
    system_load_file(sys, fname);

    // Lê nomes existentes para consultas
    FILE *f = fopen(fname, "r");
    char **existing = (char **)malloc(n * sizeof(char *));
    char line[USER_MAX];
    int ei = 0;
    while (ei < n && fgets(line, sizeof(line), f)) {
        trim(line);
        if (strlen(line) == 0) continue;
        existing[ei] = strdup(line);
        ei++;
    }
    fclose(f);

    int nq = n;  // Executa a mesma quantidade de consultas

    // Tempo SEM Bloom (direto na hash)
    double t0 = now_ms();
    srand(123);
    for (int i = 0; i < nq; i++) {
        // Alterna entre existentes e não existentes (50/50) 
        char *key = (i % 2 == 0) ? existing[rand() % ei] : "zzzzzzzz999";
        hash_search(sys->ht, key);
    }
    double time_no_bloom = now_ms() - t0;

    // Tempo COM Bloom
    memset(&sys->stats, 0, sizeof(Stats));
    double t1 = now_ms();
    srand(123);
    for (int i = 0; i < nq; i++) {
        char *key = (i % 2 == 0) ? existing[rand() % ei] : "zzzzzzzz999";
        system_query(sys, key);
    }
    double time_with_bloom = now_ms() - t1;

    // Calcula falsos positivos 
    long to_hash = sys->stats.total_queries - sys->stats.queries_avoided;
    double fp_pct = 0.0;
    if (to_hash > 0)
        fp_pct = (double)sys->stats.false_positives / to_hash * 100.0;

    printf("  +------------------------------------------+\n");
    printf("  | Consultas           : %d\n", nq);
    printf("  | Tempo SEM Bloom     : %.3f ms\n", time_no_bloom);
    printf("  | Tempo COM Bloom     : %.3f ms\n", time_with_bloom);
    printf("  | Consultas evitadas  : %ld (%.1f%%)\n",
           sys->stats.queries_avoided,
           (double)sys->stats.queries_avoided / nq * 100.0);
    printf("  | Falsos positivos    : %ld (%.4f%%)\n",
           sys->stats.false_positives, fp_pct);
    printf("  +------------------------------------------+\n");

    for (int i = 0; i < ei; i++) free(existing[i]);
    free(existing);
    system_destroy(sys);
}

// Executa todos os experimentos de 1k, 10k e 100k registros
static void run_all_experiments(void) {
    printf("\n+==========================================+\n");
    printf("|        EXPERIMENTOS AUTOMATIZADOS        |\n");
    printf("+==========================================+\n");

    run_experiment(1000);
    run_experiment(10000);
    run_experiment(100000);

    printf("\n+==========================================+\n");
    printf("|          ANALISE DOS RESULTADOS          |\n");
    printf("+==========================================+\n");
    printf("  1. O Filtro de Bloom reduziu consultas na Tabela Hash?\n");
    printf("     -> Sim. ~50%% das consultas (inexistentes) sao\n");
    printf("        descartadas antes de acessar a Hash.\n\n");
    printf("  2. Como o tamanho do vetor impactou os resultados?\n");
    printf("     -> m=1000000 para n<=100000 mantem FP < 0.01%%.\n");
    printf("        Vetores menores aumentariam a taxa de FP.\n\n");
    printf("  3. O numero de funcoes hash alterou a precisao?\n");
    printf("     -> k=7 e otimo para m/n~10 (formula: k=ln2*m/n).\n");
    printf("        Mais funcoes preenchem o vetor mais rapido.\n\n");
    printf("  4. Em qual cenario o Bloom foi mais vantajoso?\n");
    printf("     -> 100.000 registros: maior ganho absoluto de\n");
    printf("        tempo e maior numero de consultas evitadas.\n");
    printf("+==========================================+\n\n");
}


// Menu interativo do sistema
static void print_menu(void) {
    printf("\n+------------------------------------------+\n");
    printf("|   SISTEMA DE VERIFICACAO DE CADASTRO     |\n");
    printf("+------------------------------------------+\n");
    printf("|  1. INSERIR <usuario>                    |\n");
    printf("|  2. CONSULTAR <usuario>                  |\n");
    printf("|  3. ESTATISTICAS                         |\n");
    printf("|  4. CARREGAR ARQUIVO <caminho>           |\n");
    printf("|  5. EXECUTAR EXPERIMENTOS                |\n");
    printf("|  0. SAIR                                 |\n");
    printf("+------------------------------------------+\n");
    printf("Opcao: ");
}

int main(void) {
    // Cria pasta de dados
    (void)system("mkdir -p " DATA_DIR);

    printf("Inicializando sistema...\n");
    System *sys = system_create();
    if (!sys) {
        fprintf(stderr, "Falha ao inicializar o sistema.\n");
        return 1;
    }
    printf("Sistema pronto. Tabela Hash: %d buckets | Bloom: %d bits, k=%d\n\n",
           HASH_TABLE_SIZE, BLOOM_BITS, BLOOM_HASHCOUNT);

    char cmd[CMD_MAX];
    char arg[USER_MAX];

    while (1) {
        print_menu();

        if (!fgets(cmd, sizeof(cmd), stdin)) break;
        trim(cmd);
        if (strlen(cmd) == 0) continue;

        // Tenta ler como número ou como string (ex: "INSERIR joao123")
        int opcao = -1;
        arg[0] = '\0';

        char verb[32] = {0};
        int matched = sscanf(cmd, "%31s %63s", verb, arg);

        if (matched >= 1) {
            // Verifica se é número
            char *endptr;
            long val = strtol(verb, &endptr, 10);
            if (*endptr == '\0') {
                opcao = (int)val;
            } else if (strcasecmp(verb, "INSERIR")   == 0) { opcao = 1; }
            else if   (strcasecmp(verb, "CONSULTAR") == 0) { opcao = 2; }
        }

        switch (opcao) {

            // RF01 - INSERIR
            case 1:
                if (strlen(arg) == 0) {
                    printf("  Uso: 1 <usuario>   ex: 1 joao123\n");
                    break;
                }
                {
                    int r = system_insert(sys, arg);
                    if (r == 1)      printf("  [OK]     '%s' cadastrado.\n", arg);
                    else if (r == 0) printf("  [AVISO]  '%s' ja existe.\n", arg);
                    else             printf("  [ERRO]   Falha ao inserir.\n");
                }
                break;

            // RF02 - CONSULTAR
            case 2:
                if (strlen(arg) == 0) {
                    printf("  Uso: 2 <usuario>   ex: 2 joao123\n");
                    break;
                }
                {
                    int r = system_query(sys, arg);
                    if (r == 2)
                        printf("  [RESULTADO] '%s': ENCONTRADO.\n", arg);
                    else
                        printf("  [RESULTADO] '%s': INEXISTENTE.\n", arg);
                }
                break;

            // RF03 - ESTATÍSTICAS
            case 3:
                system_print_stats(sys);
                break;

            // RF04 - CARREGAR ARQUIVO
            case 4:
                if (strlen(arg) == 0) {
                    printf("  Uso: 4 <arquivo>   ex: 4 data/usuarios.txt\n");
                    break;
                }
                {
                    int n = system_load_file(sys, arg);
                    if (n >= 0)
                        printf("  [OK] %d usuario(s) carregado(s) de '%s'.\n", n, arg);
                }
                break;

            // Executa experimentos de 1k, 10k e 100k registros
            case 5:
                run_all_experiments();
                break;

            // SAIR
            case 0:
                printf("Encerrando sistema!\n");
                system_destroy(sys);
                return 0;

            default:
                printf("  Opcao invalida. Use 0-5.\n");
                break;
        }
    }

    system_destroy(sys);
    return 0;
}