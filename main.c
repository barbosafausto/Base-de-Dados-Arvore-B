#include <stdio.h>
#include "funcionalidades/funcionalidades.h"

/* ========================================================================== *
 * IDENTIFICAÇÃO DA DUPLA                                                     *
 * ========================================================================== *
 * 15512767 - José Fausto Vital Barbosa                                       *
 * 16876293 - João Pedro Boaretto                                             *
 * ========================================================================== */

/**
 * @brief Função principal que gerencia o fluxo de execução do programa.
 * O programa funciona como um interpretador de comandos, onde o primeiro
 * inteiro lido na entrada padrão define a funcionalidade (1 a 10) a ser executada.
 * * @return int 0 para execução finalizada com sucesso.
 */
int main() {

    int funcionalidade; // Define qual operação será executada
    int nBuscas;        // Variável auxiliar para armazenar a quantidade de operações em lote
    char arquivo1[64];  // Nome do primeiro arquivo de entrada/saída
    char arquivo2[64];  // Nome do segundo arquivo de entrada/saída (geralmente índice ou CSV)

    // A primeira entrada define a funcionalidade. Se não conseguir ler, encerra.
    if (scanf("%d", &funcionalidade) != 1) {
        return 0;
    }

    // Direcionamento para a funcionalidade correspondente
    switch (funcionalidade) {
        
        /**
         * Funcionalidade [1]: CREATE TABLE
         * Lê os dados de um arquivo CSV (arquivo1) e grava em um arquivo binário de dados (arquivo2).
         */
        case 1:
            scanf(" %s %s", arquivo1, arquivo2);
            createTable(arquivo1, arquivo2);
            break;

        /**
         * Funcionalidade [2]: SELECT FROM
         * Imprime todos os registros de dados armazenados no arquivo binário, ignorando os removidos.
         */
        case 2:
            scanf(" %s", arquivo1);
            selectFromTable(arquivo1);
            break;

        /**
         * Funcionalidade [3]: SELECT FROM WHERE (Busca Sequencial)
         * Recupera registros que satisfaçam um ou mais critérios de busca.
         */
        case 3:
            // Por causa da função selectWhereAB, a função selectWhere não precisa mais ser chamada aqui.
            // A função selectWhereAB será responsável por chamá-la caso não haja o campo "codEstacao"
            // entre os filtros de entrada.
            scanf(" %s %d", arquivo1, &nBuscas);
            selectWhereAB(arquivo1, NULL, nBuscas);
            break;

        /**
         * Funcionalidade [4]: DELETE WHERE
         * Realiza a remoção lógica de registros baseada em critérios de busca.
         */
        case 4:
            scanf(" %s %d", arquivo1, &nBuscas);
            deleteWhere(arquivo1, nBuscas);
            break;

        /**
         * Funcionalidade [5]: INSERT INTO
         * Insere novos registros no arquivo binário, reaproveitando espaços removidos (se houver).
         */
        case 5:
            scanf(" %s %d", arquivo1, &nBuscas);
            insertInto(arquivo1, nBuscas);
            break;

        /**
         * Funcionalidade [6]: UPDATE
         * Atualiza campos específicos de registros que satisfaçam os critérios de busca.
         */
        case 6:
            scanf(" %s %d", arquivo1, &nBuscas);
            update(arquivo1, nBuscas);
            break;

        /**
         * Funcionalidade [7]: CREATE INDEX (Árvore-B)
         * Cria um arquivo de índice primário Árvore-B a partir do arquivo de dados.
         */
        case 7:
            scanf(" %s %s", arquivo1, arquivo2);
            createIndex(arquivo1, arquivo2);
            break;

        /**
         * Funcionalidade [8]: SELECT FROM WHERE (Com Índice Árvore-B)
         * Recupera registros usando o índice caso a chave primária seja fornecida,
         * otimizando a busca.
         */
        case 8:
            scanf(" %s %s %d", arquivo1, arquivo2, &nBuscas);
            selectWhereAB(arquivo1, arquivo2, nBuscas);
            break;

        /**
         * Funcionalidade [9]: INSERT INTO (Na Árvore-B e no Arquivo de Dados)
         * Insere novos registros no arquivo de dados e atualiza o arquivo de índice 
         * Árvore-B simultaneamente para manter a consistência estrutural.
         */
        case 9:
            scanf(" %s %s %d", arquivo1, arquivo2, &nBuscas);
            insertIntoAB(arquivo1, arquivo2, nBuscas);
            break;

        /**
         * Funcionalidade [10]: DELETE WHERE (Na Árvore-B e no Arquivo de Dados)
         * Deleta registros lógicos no arquivo de dados a partir de um filtro e 
         * remove a chave correspondente do índice Árvore-B.
         */
        case 10:
            scanf(" %s %s %d", arquivo1, arquivo2, &nBuscas);
            deleteWhereAB(arquivo1, arquivo2, nBuscas);
            break;
    }

    return 0;
}