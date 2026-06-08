#ifndef REGISTRO_H

    #define REGISTRO_H

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "../fornecidas/fornecidas.h"

    #define TAM_CABECALHO 17
    #define TAM_REGISTRO 80
    #define LIXO '$'

    // --- Structs para o cabeçalho e registro

    // Cabeçalho (17 bytes)
    typedef struct {
        char status;                // '0' -> inconsistente. '1' -> consistente.
        int topo;                   // byte offset de um registro removido 
        int proxRRN;                // próximo RRN disponível
        int nroEstacoes;            // Qtd estações diferentes armazenadas
        int nroParesEstacao;        // Qtd pares de estações diferentes armazenados
    } Cabecalho;

    // Registro (80 bytes)
    typedef struct {                // 37 bytes fixos + 43 bytes variáveis
        char removido;
        int proximo;
        int codEstacao;
        int codLinha;
        int codProxEstacao;
        int distProxEstacao;
        int codLinhaIntegra;
        int codEstIntegra;
        int tamNomeEstacao;
        char nomeEstacao[85];       // Espaço na memória para trabalhar com o nomeEstacao, será tratado na hora de gravar.
        int tamNomeLinha;
        char nomeLinha[85];        // 85 bytes - valor para dar uma boa margem de segurança.
    } Registro;

    // --- Funções Auxiliares
    
    // Processa as linhas do CSV e coloca os campos na struct
    void registro_processaCSV(Registro *registro, char *pLinha);

    // Inicia o cabeçalho na funcionalidade "createTable"
    void registro_initCabecalho(Cabecalho *cabecalho);

    // Gerencia consistência do arquivo nas funcionalidades, além de escrever o cabeçalho 
    int registro_gerenciaCabecalho(Cabecalho *cabecalho, FILE *arquivoBin, int escreveConsistente, int leitura);

    // Lê o cabeçalho do arquivo
    void registro_lerCabecalho(Cabecalho *cabecalho, FILE *arquivoBin);

    // Escreve o cabeçalho no arquivo
    void registro_escreverCabecalhoBin(FILE *arquivo, Cabecalho *cabecalho);
    
    // Lê o registro do arquivo
    int registro_lerRegistroBin(FILE *arquivo, Registro *registro);

    // Escreve o registro no arquivo
    void registro_escreverRegistroBin(FILE *arquivo, Registro *registro);

    // Imprime um registro
    void registro_imprimirRegistro(Registro *registro);

    // Deleta um registro usando a remoção lógica
    void registro_deletarRegistro(Registro *registro, Cabecalho *cabecalho, FILE *arquivoBin, int offsetAtual);

    // Lê n registros da entrada e coloca todos em structs
    void registro_lerRegistro(Registro *registros);

#endif 