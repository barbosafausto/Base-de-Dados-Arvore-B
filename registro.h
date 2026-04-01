#ifndef REGISTRO_H
#define REGISTRO_H

#include <stdio.h>

#define TAM_CABECALHO 17
#define TAM_REGISTRO 80
#define LIXO '$'

// Cabeçalho (17 bytes)
typedef struct {
    char status;                // '0' -> inconsistente. '1' -> consistente.
    int topo;                   // byte offset de um registro removido )
    int proxRRN;                // próximo RRN disponível
    int nroEstacoes;            // Qtd estações diferentes armazenadas
    int nroParesEstacao;        // Qtd pares de estações diferentes armazenados
} Cabecalho;

// Registro (80 bytes)
typedef struct {             // 37 bytes fixos + 43 bytes variáveis
    char removido;
    int proximo;
    int codEstacao;
    int codLinha;
    int codProxEstacao;
    int distProxEstacao;
    int codLinhaIntegra;
    int codEstIntegra;
    int tamNomeEstacao;
    char nomeEstacao[64];   // Espaço na memória para trabalhar com o nomeEstacao, será tratado na hora de gravar.
    int tamNomeLinha;
    char nomeLinha[64];     // 64 bytes - valor para dar uma boa margem de segurança.
} Registro;

void initCabecalho(Cabecalho *cabecalho);
void escreverCabecalhoBin(FILE *arquivo, Cabecalho *cabecalho);
int lerRegistroBin(FILE *arquivo, Registro *registro);
void escreverRegistroBin(FILE *arquivo, Registro *registro);
void imprimirRegistro(Registro *registro);

#endif 