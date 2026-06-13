#ifndef FUNCIONALIDADES_H
#define FUNCIONALIDADES_H

    #include "../registro/registro.h"
    #include "../utils/utils.h"
    #include "../fornecidas/fornecidas.h"
    #include "../arvoreb/arvoreb.h"

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>

    /* ========================================================================== *
     * FUNCIONALIDADES BÁSICAS (OPERAÇÕES EM ARQUIVO DE DADOS)                    *
     * ========================================================================== */

    /**
     * @brief [Funcionalidade 1] Lê registros de um arquivo CSV e os grava em um arquivo binário.
     * @param nomeArquivoCSV Nome do arquivo de texto de entrada.
     * @param nomeArquivoBin Nome do arquivo binário de saída.
     */
    void createTable(char *nomeArquivoCSV, char *nomeArquivoBin);

    /**
     * @brief [Funcionalidade 2] Imprime todos os registros válidos armazenados no arquivo binário.
     * @param nomeArquivoBin Nome do arquivo binário de dados.
     */
    void selectFromTable(char *nomeArquivoBin);

    /**
     * @brief [Funcionalidade 3] Realiza busca sequencial de registros baseada em filtros de busca.
     * Adaptada para receber o ponteiro do arquivo aberto, permitindo seu reaproveitamento
     * pela funcionalidade [8] para buscas que não possuem a chave primária (codEstacao).
     * @param arquivoBin Ponteiro para o arquivo binário já aberto.
     * @param busca Estrutura contendo os filtros (campos e valores) desejados.
     */
    void selectWhere(FILE *arquivoBin, Busca *busca);

    /**
     * @brief [Funcionalidade 4] Realiza a remoção lógica de registros baseada em filtros.
     * @param nomeArquivoBin Nome do arquivo binário de dados.
     * @param nRemocoes Quantidade de operações de exclusão a serem executadas.
     */
    void deleteWhere(char *nomeArquivoBin, int nRemocoes);

    /**
     * @brief [Funcionalidade 5] Insere novos registros reaproveitando espaços removidos (via pilha) ou no fim do arquivo.
     * @param nomeArquivoBin Nome do arquivo binário de dados.
     * @param nInsercoes Quantidade de registros a serem inseridos.
     */
    void insertInto(char *nomeArquivoBin, int nInsercoes);

    /**
     * @brief [Funcionalidade 6] Atualiza (sobrescreve) campos específicos de registros que satisfaçam as condições de busca.
     * @param nomeArquivoBin Nome do arquivo binário de dados.
     * @param nAtualizacoes Quantidade de operações de atualização.
     */
    void update(char *nomeArquivoBin, int nAtualizacoes);


    /* ========================================================================== *
     * FUNCIONALIDADES OTIMIZADAS (OPERAÇÕES INTEGRADAS COM ÁRVORE-B)             *
     * ========================================================================== */

    /**
     * @brief [Funcionalidade 7] Gera um arquivo de Índice Árvore-B a partir do arquivo de dados.
     * @param nomeArquivoDadosBin Nome do arquivo de dados base.
     * @param nomeArquivoIndiceBin Nome do arquivo de índice que será criado.
     */
    void createIndex(char* nomeArquivoDadosBin, char* nomeArquivoIndiceBin);

    /**
     * @brief [Funcionalidade 8] Busca registros priorizando o acesso rápido via Árvore-B.
     * Delega a pesquisa para a varredura sequencial (selectWhere [3]) caso a chave primária não seja informada.
     * @param nomeArquivoDadosBin Nome do arquivo binário de dados.
     * @param nomeArquivoIndiceBin Nome do arquivo binário de índice.
     * @param nBuscas Quantidade de pesquisas em lote.
     */
    void selectWhereAB(char* nomeArquivoDadosBin, char* nomeArquivoIndiceBin, int nBuscas);

    /**
     * @brief [Funcionalidade 9] Insere registros no arquivo de dados mantendo a Árvore-B sincronizada com o arquivo de dados.
     * @param nomeArquivoDadosBin Nome do arquivo de dados.
     * @param nomeArquivoIndiceBin Nome do arquivo de índice.
     * @param nInsercoes Quantidade de registros a serem inseridos.
     */
    void insertIntoAB(char *nomeArquivoDadosBin, char *nomeArquivoIndiceBin, int nInsercoes);

    /**
     * @brief [Funcionalidade 10] Deleta registros de forma sincronizada em ambos os arquivos, priorizando busca na árvore se possível.
     * @param nomeArquivoDadosBin Nome do arquivo de dados.
     * @param nomeArquivoIndiceBin Nome do arquivo de índice.
     * @param nRemocoes Quantidade de operações de remoção.
     */
    void deleteWhereAB(char *nomeArquivoDadosBin, char *nomeArquivoIndiceBin, int nRemocoes);

#endif