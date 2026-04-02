#ifndef UTILS_H
    #define UTILS_H

    #include "registro.h"
    #include "fornecidas.h"

    //     Structs para auxiliar na funcionalidade createTable
    // nomeEstacao (para nroEstacoes)
    typedef struct nodeNome {
        char *nome;
        struct nodeNome *prox;
    }NodeNome;

    // paresEstacao (nroParesEstacao)
    typedef struct nodePares {
        int cod1;
        int cod2;
        struct nodePares *prox;
    }NodePares;


    //      Structs para auxiliar na funcionalidade de busca com filtros
    // Campos valorString e valorInt para diferenciar entre leitura com aspas ("") e leitura de NULO ou números.
    // isString serve pra facilitar na hora da comparação.
    typedef struct {
        char nomeCampo[20];
        char valorString[64];
        int valorInt;
        int isString; // 1 - String, 0 - Int ou NULO
    } Filtro;

    // Struct para ajudar na leitura de todos os n filtros.
    typedef struct {
        int mFiltros;
        Filtro *filtro;
    } Busca;


    int adicionarEstacaoUnica(NodeNome **head, char *nome);
    int adicionarParUnico(NodePares **head, int cod1, int cod2);
    void utils_contaNroEstacoesNroPares(Cabecalho *cabecalho, FILE *arquivoBin);

    void liberarUtils(NodeNome *headNomes, NodePares *headPares);

    void recebeFiltros(Busca *busca, int nBuscas);
    int compararRegistroComFiltros(Registro *registro, Busca *busca);
    void utils_atualizarRegistroComFiltros(Busca busca, FILE *arquivoBin, int offsetAtual);

#endif