#ifndef REGISTRO_H
#define REGISTRO_H

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include "../fornecidas/fornecidas.h"

    /* ========================================================================== *
     * MACROS E CONSTANTES GERAIS                                                 *
     * ========================================================================== */

    #define TAM_CABECALHO 17
    #define TAM_REGISTRO 80
    #define LIXO '$'

    /* ========================================================================== *
     * ESTRUTURAS DE DADOS                                                        *
     * ========================================================================== */

    /**
     * @brief Estrutura que representa o Registro de Cabeçalho do arquivo de dados.
     * O tamanho exato do registro de cabeçalho no disco deve ser de 17 bytes.
     */
    typedef struct {
        char status;                // '0' -> inconsistente. '1' -> consistente.
        int topo;                   // byte offset de um registro removido (topo da pilha LIFO).
        int proxRRN;                // próximo RRN disponível para inserção no fim do arquivo.
        int nroEstacoes;            // Quantidade de estações diferentes armazenadas.
        int nroParesEstacao;        // Quantidade de pares de estações diferentes armazenados.
    } Cabecalho;

    /**
     * @brief Estrutura que representa um Registro de Dados na memória principal.
     * Ocupa 80 bytes físicos no disco (37 bytes de campos fixos + 43 bytes dinâmicos + Lixo).
     */
    typedef struct {                
        char removido;              // '0' -> ativo, '1' -> logicamente removido.
        int proximo;                // byte offset do próximo registro removido (pilha).
        
        int codEstacao;
        int codLinha;
        int codProxEstacao;
        int distProxEstacao;
        int codLinhaIntegra;
        int codEstIntegra;
        
        int tamNomeEstacao;
        char nomeEstacao[85];       // Buffer de RAM generoso. Apenas tamNomeEstacao bytes vão ao disco.
        
        int tamNomeLinha;
        char nomeLinha[85];         // Buffer de segurança para processamento das strings.
    } Registro;


    /* ========================================================================== *
     * PROTÓTIPOS: GERENCIAMENTO DO CABEÇALHO (I/O E ESTADO)                      *
     * ========================================================================== */

    // Inicia o cabeçalho na funcionalidade "createTable" com valores padrão.
    void registro_initCabecalho(Cabecalho *cabecalho);

    // Lê os metadados do cabeçalho do arquivo binário para a memória principal.
    void registro_lerCabecalho(FILE *arquivoBin, Cabecalho *cabecalho);

    // Escreve a estrutura do cabeçalho da RAM para o arquivo binário (17 bytes).
    void registro_escreverCabecalhoBin(FILE *arquivo, Cabecalho *cabecalho);
    
    // Gerencia consistência do arquivo nas funcionalidades e regrava o cabeçalho.
    int registro_gerenciaCabecalho(Cabecalho *cabecalho, FILE *arquivoBin, int escreveConsistente, int leitura);


    /* ========================================================================== *
     * PROTÓTIPOS: PROCESSAMENTO DE ENTRADA (CSV E STDIN)                         *
     * ========================================================================== */

    // Processa uma linha bruta do CSV, fatiando os campos e populando a struct.
    void registro_processaCSV(Registro *registro, char *pLinha);

    // Lê os campos de um registro formatado diretamente da entrada padrão (teclado).
    void registro_lerRegistro(Registro *registros);


    /* ========================================================================== *
     * PROTÓTIPOS: MANIPULAÇÃO DE REGISTROS (I/O, IMPRESSÃO E DELEÇÃO)            *
     * ========================================================================== */

    // Lê um registro do disco abstraindo o controle de campos variáveis e pulo de lixo.
    int registro_lerRegistroBin(FILE *arquivo, Registro *registro);

    // Formata e escreve o registro no disco garantindo o preenchimento de LIXO até 80 bytes.
    void registro_escreverRegistroBin(FILE *arquivo, Registro *registro);

    // Imprime os campos formatados, tratando os identificadores numéricos e nulos.
    void registro_imprimirRegistro(Registro *registro);

    // Aplica a remoção lógica (tombstone), manipula a pilha e volta o cursor fisicamente.
    void registro_deletarRegistro(Registro *registro, Cabecalho *cabecalho, FILE *arquivoBin, int offsetAtual);

#endif