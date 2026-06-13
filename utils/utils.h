#ifndef UTILS_H
#define UTILS_H

    #include "../registro/registro.h"
    #include "../fornecidas/fornecidas.h"

    /* ========================================================================== *
     * ESTRUTURAS DE DADOS: MÉTRICAS E CONTROLE DE UNICIDADE                      *
     * ========================================================================== */

    /**
     * @brief Nó de lista encadeada para controle de unicidade do nomeEstacao.
     * Auxilia na contagem do campo 'nroEstacoes' do cabeçalho sem lotar a RAM.
     */
    typedef struct nodeNome {
        char *nome;
        struct nodeNome *prox;
    } NodeNome;

    /**
     * @brief Nó de lista encadeada para controle de unicidade de pares de estações.
     * Auxilia na contagem do campo 'nroParesEstacao' do cabeçalho de forma bidirecional.
     */
    typedef struct nodePares {
        int cod1;
        int cod2;
        struct nodePares *prox;
    } NodePares;


    /* ========================================================================== *
     * ESTRUTURAS DE DADOS: FILTROS E BUSCAS                                      *
     * ========================================================================== */

    /**
     * @brief Estrutura que representa um único critério/campo de busca.
     * Diferencia internamente strings com aspas, valores inteiros numéricos e valores NULO.
     */
    typedef struct {
        char nomeCampo[20];
        char valorString[64];
        int valorInt;
        int isString; // Flag: 1 para String, 0 para Inteiro ou NULO
    } Campo;

    /**
     * @brief Estrutura agregadora que armazena um lote de campos de busca.
     * Centraliza a leitura das condições WHERE e SET nas funcionalidades.
     */
    typedef struct {
        int mCampos;    // Quantidade de filtros/campos exigidos na operação
        Campo *campo;   // Vetor dinâmico contendo as propriedades de cada filtro
    } Busca;


    /* ========================================================================== *
     * PROTÓTIPOS: GERENCIAMENTO DE LISTAS (CONTADORES ÚNICOS)                    *
     * ========================================================================== */

    // Incrementa a métrica virtual inserindo a estação apenas se for inédita.
    int utils_adicionarEstacaoUnica(NodeNome **head, char *nome);

    // Incrementa a métrica virtual inserindo o par bidirecional se for inédito e não nulo.
    int utils_adicionarParUnico(NodePares **head, int cod1, int cod2);

    // Varre o arquivo binário para estabilizar as métricas do cabeçalho após mutações.
    void utils_contaNroEstacoesNroPares(Cabecalho *cabecalho, FILE *arquivoBin, int Delete);

    // Realiza a limpeza de memória profunda (Heap) de todos os nós das listas auxiliares.
    void utils_liberarUtils(NodeNome *headNomes, NodePares *headPares);


    /* ========================================================================== *
     * PROTÓTIPOS: MOTOR DE BUSCA E ATUALIZAÇÃO                                   *
     * ========================================================================== */

    // Instancia o pacote de busca lendo e interpretando os tipos de dados da STDIN.
    int utils_recebeCampos(Busca *busca);

    // Motor de comparação estrita (AND lógico) entre um registro físico e os critérios do usuário.
    int utils_compararRegistroComFiltros(Registro *registro, Busca *busca);

    // Executa a injeção física de dados específicos sobre um registro já validado.
    void utils_atualizarRegistroComFiltros(Busca busca, FILE *arquivoBin, int offsetAtual);

#endif