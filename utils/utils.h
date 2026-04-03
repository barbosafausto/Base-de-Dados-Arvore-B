#ifndef UTILS_H
    #define UTILS_H

    #include "../registro/registro.h"
    #include "../fornecidas/fornecidas.h"

    // --- Structs para auxiliar contagem de estações e pares
    // nomeEstacao (nroEstacoes)
    typedef struct nodeNome {
        char *nome;
        struct nodeNome *prox;
    } NodeNome;

    // paresEstacao (nroParesEstacao)
    typedef struct nodePares {
        int cod1;
        int cod2;
        struct nodePares *prox;
    } NodePares;


    // --- Structs para auxiliar na funcionalidade de busca com campos
    // Campos valorString e valorInt para diferenciar entre leitura com aspas ("") e leitura de NULO ou números.
    // isString serve pra facilitar na hora da comparação.
    typedef struct {
        char nomeCampo[20];
        char valorString[64];
        int valorInt;
        int isString; // 1 - String, 0 - Int ou NULO
    } Campo;

    // Struct para ajudar na leitura de todas as n buscas, seja seleção, remoção ou atualização.
    typedef struct {
        int mCampos;
        Campo *campo;
    } Busca;


    //  --- Funções auxiliares

    // Incrementa nroEstacoes se a atual for única
    int utils_adicionarEstacaoUnica(NodeNome **head, char *nome);

    // Incrementa o nroParesEstacoes se o códigos atuais forem únicos e não-nulos
    int utils_adicionarParUnico(NodePares **head, int cod1, int cod2);

    // Percorre o arquivo nas função Insert e Delete para atualizar o nroEstacoes e nroParesEstacoes
    void utils_contaNroEstacoesNroPares(Cabecalho *cabecalho, FILE *arquivoBin, int Delete);

    // Liberação da memória alocada para contar as estações e os pares 
    void utils_liberarUtils(NodeNome *headNomes, NodePares *headPares);

    // Função extremamente útil: recebe campos tratando strings e valores nulos
    void utils_recebeCampos(Busca *busca, int nBuscas);

    // Função extremamente útil 2: comparar registros com filtros
    int utils_compararRegistroComFiltros(Registro *registro, Busca *busca);

    // Atualiza registro com base nos filtros da funcionalidade "update"
    void utils_atualizarRegistroComFiltros(Busca busca, FILE *arquivoBin, int offsetAtual);

#endif