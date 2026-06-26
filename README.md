# Sistema de Consulta Eficiente com Tabela Hash e Filtro de Bloom

Sistema de verificação de cadastro de usuários utilizando **Tabela Hash com encadeamento externo** e **Filtro de Bloom** como estrutura probabilística para acelerar consultas.

---

```markdown
# Sistema de Consulta Eficiente com Tabela Hash e Filtro de Bloom

Sistema de verificação de cadastro de usuários utilizando **Tabela Hash com encadeamento externo** e **Filtro de Bloom** como estrutura probabilística para acelerar consultas.

---

## Estrutura do Projeto

projeto/
├── data/
│   └── usuarios.txt     # Arquivo com massa de dados para testes em lote
├── src/
│   ├── bloom.c          # Implementação dos algoritmos do Filtro de Bloom
│   ├── bloom.h          # Protótipos, constantes e macros bitwise do Bloom
│   ├── hash.c           # Implementação da Tabela Hash com encadeamento
│   ├── hash.h           # Estrutura dos nós, buckets e assinaturas da Hash
│   └── main.c           # Ponto de entrada, menu CLI e execução dos experimentos
├── README.md            # Manual de instruções, como compilar e executar

```

---

## Compilação

### Pré-requisitos

* Compilador GCC 
* Biblioteca matemática (`libm`)

### Compilar

Abra o terminal na pasta raiz do projeto e execute o comando abaixo:

```bash
gcc -O2 -o sistema src/main.c src/hash.c src/bloom.c -lm

```

Esse comando irá gerar o arquivo executável chamado `sistema.exe`.

---

## Execução

Após a compilação, o arquivo executável será gerado na raiz do projeto. Execute o comando correspondente ao seu terminal/sistema operacional:

| Sistema / Terminal | Comando para Executar |
| :--- | :--- |
| **Windows (Prompt de Comando - CMD)** | `sistema.exe` |
| **Windows (PowerShell)** | `.\sistema.exe` |
| **Linux (Terminal / Bash)** | `./sistema` |

---

## Menu de Opções

```text
+------------------------------------------+
|   SISTEMA DE VERIFICACAO DE CADASTRO     |
+------------------------------------------+
|  1. INSERIR <usuario>                    |
|  2. CONSULTAR <usuario>                  |
|  3. ESTATISTICAS                         |
|  4. CARREGAR ARQUIVO <caminho>           |
|  5. EXECUTAR EXPERIMENTOS                |
|  0. SAIR                                 |
+------------------------------------------+

```

---

## Formato de Entrada

### Inserção manual (RF01)

```text
1 joao123
1 maria98

```

Ou com a palavra-chave:

```text
INSERIR joao123

```

### Consulta (RF02)

```text
2 joao123
CONSULTAR ana777

```

### Carregar arquivo (RF04)

```text
4 data/usuarios.txt

```

**Formato do arquivo**: um identificador por linha.

```text
joao123
maria98
pedro45

```

### Executar experimentos automáticos

```text
5

```

Gera arquivos com 1.000, 10.000 e 100.000 registros no formato `[8letras][3números]` (ex: `islaifda122`) e exibe a tabela comparativa de desempenho.

---

## Exemplos de Execução

### Exemplo 1 — Inserir e consultar

```text
Opcao: 1 joao123
  [OK] 'joao123' cadastrado.

Opcao: 2 joao123
  [RESULTADO] 'joao123': ENCONTRADO.

Opcao: 2 ana777
  [RESULTADO] 'ana777': INEXISTENTE.

```

### Exemplo 2 — Carregar arquivo e ver estatísticas

```text
Opcao: 4 data/usuarios.txt
  [OK] 10 usuario(s) carregado(s) de 'data/usuarios.txt'.

Opcao: 3
  Elementos cadastrados : 10
  Total de consultas    : 0
  ...

```

### Exemplo 3 — Experimentos

```text
Opcao: 5
[EXPERIMENTO] 1000 registros
  Tempo SEM Bloom : 0.073 ms
  Tempo COM Bloom : 0.116 ms
  ...

```

---

## Parâmetros das Estruturas

| Parâmetro | Valor | Justificativa |
| --- | --- | --- |
| `HASH_TABLE_SIZE` | 131.101 (primo) | Load factor ~0.77 para n=100.000 |
| `BLOOM_BITS` (m) | 1.000.000 | FP teórico < 0,01% para n=100.000, k=7 |
| `BLOOM_HASHCOUNT` (k) | 7 | Ótimo: k = ln(2) × m/n ≈ 6,93 |

---

## Divisão da Equipe

| Integrante | Responsabilidade | Arquivo(s) |
| --- | --- | --- |
| Micaely Melo | Tabela Hash | `hash.c`, `hash.h` |
| Naama Flávia | Filtro de Bloom | `bloom.c`, `bloom.h` |
| Alex Estêvão | Integração e avaliação | `main.c`, `relatorio` |
