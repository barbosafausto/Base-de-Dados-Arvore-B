#ifndef ARVOREB_H
    #define  ARVOREB_H

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    /* ========================================================================== *
     * MACROS E CONSTANTES GERAIS                                                 *
     * ========================================================================== */

    // O tamanho dos registros de dados (nós da árvore) deve ser de 53 bytes.
    #define TAM_NO 53
    
    // A ordem da árvore-B é 4, ou seja, m=4. 
    // Portanto, um nó terá 3 chaves no máximo e 4 descendentes.
    #define ORDEM_ARVORE 4

    // A ocupação mínima é o teto da divisão da ordem por 2, subtraído de 1.
    // Uma página folha possui no mínimo teto(m/2)-1 chaves.
    // Teto de a/b: (a + b - 1)/b
    #define OCUPACAO_MINIMA ((ORDEM_ARVORE + 2 - 1) / 2) - 1

    /* ========================================================================== *
     * ESTRUTURAS DE DADOS                                                        *
     * ========================================================================== */

    /**
     * @brief Estrutura que representa o Registro de Cabeçalho do arquivo de índice.
     * O tamanho do registro de cabeçalho deve ser de 17 bytes.
     */
    typedef struct CabecalhoAB {

        char status;      // Indica a consistência do arquivo: '0' inconsistente, '1' consistente.
        int noRaiz;       // Armazena o RRN do nó (página) raiz. Quando vazia, noRaiz = -1.
        int topo;         // Armazena o RRN de um registro logicamente removido (pilha), ou -1.
        int proxRRN;      // Armazena o valor do próximo RRN a ser usado para conter um nó.
        int nroNos;       // Armazena o número de nós (páginas) do índice árvore-B.

    } CabecalhoAB;
    
    /**
     * @brief Estrutura auxiliar para agrupar uma chave de busca e suas referências.
     * Facilita a movimentação de dados ordenados na memória primária.
     */
    typedef struct Estacao {

        int chave;          // Chave de busca (codEstacao).
        int Pr;             // Referência para o registro no arquivo de dados (byte offset).
        int P2;             // Ponteiro para a subárvore à direita da chave na árvore-B.

    } Estacao;

    /**
     * @brief Estrutura que representa um Nó (Página / Registro de Dados) da Árvore-B.
     * O tamanho exato do nó gravado deve ser de 53 bytes.
     */
    typedef struct No {

        char removido;      // Indica se o nó está logicamente removido: '1' removido, '0' ativo.
        
        int proximo;        // Armazena o RRN do próximo nó logicamente removido (pilha).
        int tipoNo;         // Indica o tipo: -1 (folha), 0 (raiz) ou 1 (intermediário).
        int nroChaves;      // Indica o número de chaves atualmente presentes no nó.

        /*
        Abaixo, seguem as chaves (codEstacao),
        suas respectivas referências (Pr) para arquivo de dados
        e seus filhos na árvore-B (P2).

        Usamos vetores para poder generalizar um pouco a árvore. :D
        */
        Estacao estacao[ORDEM_ARVORE-1];

        /*
        Vamos considerar P1 como o filho adotivo da estacao[0] de cada nó.
        Isso acontece porque as estações possuem apenas filhos direitos.
        O valor -1 deve ser usado para denotar que um ponteiro é nulo.
        */ 
        int P1; 

    } NO;

    /* ========================================================================== *
     * PROTÓTIPOS: GERENCIAMENTO DE CABEÇALHO                                     *
     * ========================================================================== */

    // Função responsável por inicializar o cabeçalho da árvore-B com valores padrão.
    void arvoreb_initCabecalho(CabecalhoAB *cabecalhoAB);

    // Lê os 17 bytes do cabeçalho do arquivo binário para a struct na memória.
    void arvoreb_lerCabecalhoBin(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB);

    // Escreve a struct cabeçalho no byte 0 do arquivo binário de índice.
    void arvoreb_escreverCabecalhoBin(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB);

    // Gerencia a consistência do cabeçalho ao abrir ou fechar o arquivo.
    //  As flags desta função funcionam da seguinte forma:
    //  escreverConsistente = 1:                         setar o arquivo para consistente e escrever cabecalho.
    //  escreverConsistente = 0 e abertoParaLeitura = 1: apenas verificar se está inconsistente, pois o arquivo foi aberto para leitura.
        // Retorna 0 se o arquivo estiver inconsistente nesse caso; retorna 1 em qualquer outro caso.
    //  escreverConsistente = 0 e abertoParaleitura = 0: setar o arquivo para inconsistente no disco, pois o arquivo foi aberto para escrita.
    int arvoreb_gerenciaCabecalho(CabecalhoAB *cabecalho, FILE *arquivoIndiceBin, int escreverConsistente, int abertoParaLeitura);


    /* ========================================================================== *
     * PROTÓTIPOS: MANIPULAÇÃO DE NÓS (I/O)                                       *
     * ========================================================================== */

    // Lê uma página (nó) específica da memória secundária dado o seu RRN.
    NO arvoreb_lerNoBin(FILE *arquivoIndiceBin, int RRN);

    // Aloca e inicializa as variáveis de um novo nó em memória primária.
    NO arvoreb_criarNoBin(CabecalhoAB *cabecalhoAB, int tipoNo, int P1, Estacao *estacao);

    // Grava os dados de um nó da RAM para o disco, respeitando os 53 bytes exigidos.
    void arvoreb_escreverNoBin(FILE *arquivoIndiceBin, NO *node, int RRN);


    /* ========================================================================== *
     * PROTÓTIPOS: FUNCIONALIDADE DE BUSCA                                        *
     * ========================================================================== */

    // Inicia a pesquisa de uma chave na árvore e retorna sua referência (offset) no arquivo de dados.
    int arvoreb_buscar(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int chave);

    // Função recursiva interna que navega pelas páginas da árvore em busca da chave.
    int arvoreb_buscarRecursivo(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int nodeRRN, int chave);

    // Busca binária executada dentro de uma página para encontrar a chave ou o caminho de descida.
    Estacao* arvoreb_buscaBinaria(NO *node, int chave);


    /* ========================================================================== *
     * PROTÓTIPOS: FUNCIONALIDADE DE INSERÇÃO                                     *
     * ========================================================================== */

    // Função principal que engatilha o processo de inserção de uma nova chave na árvore.
    void arvoreb_inserir(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, Estacao estacaoParaInserir);

    // Função recursiva interna que desce a árvore, realiza a inserção e propaga eventuais splits para cima.
    Estacao arvoreb_inserirRecursivo(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int nodeRRN, Estacao estacaoParaInserir);

    // Realiza o particionamento (split) de uma página lotada, promovendo a chave mediana e criando o novo nó direito.
    Estacao arvoreb_split(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, Estacao *estacaoParaInserir, NO *node, int nodeRRN);

    // Desloca e insere ordenadamente uma chave no vetor em memória (Insertion Sort).
    void arvoreb_ordenaNo(Estacao *estacoes, Estacao *estacaoParaInserir, int nroChaves);


    /* ========================================================================== *
     * PROTÓTIPOS: FUNCIONALIDADE DE REMOÇÃO                                      *
     * ========================================================================== */

    void arvoreb_remover(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int chave);
    int arvoreb_removerRecursivo(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int rrnAtual, int chave);
    int arvoreb_encontrarPosicao(NO *node, int chave);
    int arvoreb_getFilho(NO *node, int pos);
    void arvoreb_setFilho(NO *node, int pos, int rrn);
    void arvoreb_removerChaveDoNo(NO *node, int pos);
    Estacao arvoreb_buscarSucessora(FILE *arquivoIndiceBin, int rrnSubarvoreDireita);
    void arvoreb_empilharNoRemovido(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int rrnRemovido);
    void arvoreb_corrigirUnderflow(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int rrnPai, NO *pai, int posFilho);
    int arvoreb_redistribuirDireita(FILE *arquivoIndiceBin, int rrnPai, NO *pai, int posFilho, int rrnFilho, NO *filho, int rrnDir);
    int arvoreb_redistribuirEsquerda(FILE *arquivoIndiceBin, int rrnPai, NO *pai, int posFilho, int rrnFilho, NO *filho, int rrnEsq);
    int arvoreb_concatenarEsquerda(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int rrnPai, NO *pai, int posFilho, int rrnFilho, NO *filho, int rrnEsq);
    int arvoreb_concatenarDireita(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int rrnPai, NO *pai, int posFilho, int rrnFilho, NO *filho, int rrnDir);
    void arvoreb_ajustarRaiz(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB);


#endif