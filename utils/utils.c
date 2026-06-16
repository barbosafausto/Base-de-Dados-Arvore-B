#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================== *
 * LISTAS ENCADEADAS E GERENCIAMENTO DE MEMÓRIA                               *
 * ========================================================================== */

/**
 * @brief Adiciona o nome de uma estação a uma lista encadeada, garantindo unicidade.
 * Dessa forma, mesmo que o arquivo CSV seja muito grande, a memória usada para armazenar
 * os nomes das estações será relativamente pequena, evitando dados duplicados na RAM.
 * @param head Ponteiro duplo para a cabeça da lista de nomes de estações.
 * @param nome String contendo o nome da estação a ser avaliada/inserida.
 * @return int 1 se a estação era inédita e foi inserida; 0 se já existia ou era inválida/nula.
 */
int utils_adicionarEstacaoUnica(NodeNome **head, char *nome) {

    if(nome == NULL || nome[0] == '\0' || strcmp(nome, "NULO") == 0) return 0;
    
    // --- Verifica se já existe ---
    NodeNome *cur = *head;
    while(cur != NULL) {
        if(strcmp(cur->nome, nome) == 0) {
            return 0; // Estação já existe
        }
        cur = cur->prox;
    }

    // --- Verifica se não existe ---
    NodeNome *novo = (NodeNome *) malloc(sizeof(NodeNome));
    if(novo == NULL) {
        printf("Erro de alocação de memória para nome de estação.");
        return 0;
    }

    novo->nome = strdup(nome);
    novo->prox = *head; // Insere no início (mais fácil, já tem o head e não precisa de ordem)
    *head = novo;

    // Estação não existe e foi inserida
    return 1; 
}

/**
 * @brief Adiciona um par de estações (origem/destino) à lista, ignorando a ordem bidirecional: (A, B) = (B, A)
 * @param head Ponteiro duplo para a cabeça da lista de pares.
 * @param cod1 Código inteiro da primeira estação.
 * @param cod2 Código inteiro da segunda estação.
 * @return int 1 se o par for inédito e adicionado; 0 se já existir ou for inválido.
 */
int utils_adicionarParUnico(NodePares **head, int cod1, int cod2) {

    // Começamos apontando pro começo da lista
    NodePares *cur = *head;
    
    // Par inválido
    if (cod1 == -1 || cod2 == -1) return 0;

    // --- Busca linear para ver se o par existe ---
    while(cur != NULL) {
        // Nessa condição, não importa a ordem (A->B == B->A)
        if((cur->cod1 == cod1 && cur->cod2 == cod2) || (cur->cod1 == cod2 && cur->cod2 == cod1)) {
            return 0; // Par já existe 
        }
        cur = cur->prox;
    }

    // --- Se não existe, prosseguimos ---
    NodePares *novo = (NodePares *) malloc(sizeof(NodePares));
    if(novo == NULL) {
        printf("Erro de alocação de memória para par de estações.");
        return 0;
    }

    novo->cod1 = cod1;
    novo->cod2 = cod2;
    novo->prox = *head; // Insere no início
    *head = novo;

    // Par não existe e foi inserido
    return 1; 
}

/**
 * @brief Percorre e desaloca da memória RAM todos os nós das listas de controle.
 * @param headNomes Cabeça da lista encadeada de strings.
 * @param headPares Cabeça da lista encadeada de pares inteiros.
 */
void utils_liberarUtils(NodeNome *headNomes, NodePares *headPares) {

    // --- Libera lista de nomes ---
    NodeNome *curNomes = headNomes;
    while(curNomes != NULL) {
        NodeNome *temp = curNomes;
        curNomes = curNomes->prox;
        free(temp->nome); // Libera a string alocada por strdup
        free(temp);       // Libera o nó
    }

    // --- Libera lista de pares ---
    NodePares *curPares = headPares;
    while(curPares != NULL) {
        NodePares *temp = curPares;
        curPares = curPares->prox;
        free(temp); // Libera o nó
    }
}


/* ========================================================================== *
 * MÉTRICAS E RECONTAGEM                                                      *
 * ========================================================================== */

/**
 * @brief Varre o arquivo binário recalculando a métrica de pares e, opcionalmente, estações.
 * Usado após deleções ou inserções em lote para estabilizar o cabeçalho.
 * @param cabecalho Ponteiro para a struct do cabeçalho que terá suas métricas sobrescritas.
 * @param arquivoBin Descritor de arquivo aberto.
 * @param Delete Flag booleana: 1 se for chamado após deleções (reconta nomes); 0 se após inserções.
 */
void utils_contaNroEstacoesNroPares(Cabecalho *cabecalho, FILE *arquivoBin, int Delete) {

    // Volta cursor para o início, pós cabeçalho
    fseek(arquivoBin, TAM_CABECALHO, SEEK_SET);

    // --- Variáveis de contagem ---
    int nroEstacoes = 0;
    int nroParesEstacao = 0;

    // Registro que será lido para avaliar as contagens
    Registro *registro = (Registro*) malloc(sizeof(Registro));

    // Lista de nomes (estacões únicas)
    NodeNome *n_head = NULL;
    
    // Lista de pares únicos
    NodePares *p_head = NULL;
    
    // Leitura de registro enquanto não alcançar o fim do arquivo
    while(registro_lerRegistroBin(arquivoBin, registro) != -1) {

        // Ignora remoções
        if (registro->removido == '1') continue;

        // Atualização dos pares
        if (utils_adicionarParUnico(&p_head, registro->codEstacao, registro->codProxEstacao))
            nroParesEstacao++;
        
        // Se não for a funcionalidade delete, não precisamos atualizar as estações, apenas os pares.
        if (!Delete) continue;

        // Atualização das estações (funcionalidade de inserção)
        if (utils_adicionarEstacaoUnica(&n_head, registro->nomeEstacao))
            nroEstacoes++;
    }

    // --- Cabeçalho atualizado ---
    cabecalho->nroParesEstacao = nroParesEstacao;
    if (Delete)
        cabecalho->nroEstacoes = nroEstacoes;

    // --- Liberação da memória heap ---
    free(registro);
    utils_liberarUtils(n_head, p_head);
}


/* ========================================================================== *
 * TRATAMENTO DE FILTROS E BUSCAS                                             *
 * ========================================================================== */

/**
 * @brief Lê a quantidade e o formato dos filtros da entrada padrão.
 * Popula dinamicamente a estrutura de Busca e identifica se a chave primária está entre os campos.
 * @param busca Ponteiro para a estrutura genérica de busca que apontará para os campos
 * @return int Retorna o 'codEstacao' se ele constar entre os filtros, caso contrário retorna -1.
 */
int utils_recebeCampos(Busca *busca){

    // Variável de retorno 
    int codEstacao = -1;

    // --- Número de campos e alocação ---
    scanf("%d", &busca->mCampos);
    busca->campo = (Campo*) malloc(busca->mCampos * sizeof(Campo));

    // --- Leitura dos campos ---
    for (int i = 0; i < busca->mCampos; i++) {

        // Nome do campo
        scanf(" %s", busca->campo[i].nomeCampo);        

        // --- Campo string ---
        if (strcmp(busca->campo[i].nomeCampo, "nomeEstacao") == 0 ||
            strcmp(busca->campo[i].nomeCampo, "nomeLinha") == 0) {
             
                ScanQuoteString(busca->campo[i].valorString);
                busca->campo[i].isString = 1;

        }
        // --- Campo int ou NULO ---
        else {
            
            char valor[85];
            scanf(" %s", valor);

            busca->campo[i].valorInt = (strcmp(valor, "NULO") == 0) ? -1 : atoi(valor);
            busca->campo[i].isString = 0;
            
            // Vejo se é um codEstacao
            if (strcmp(busca->campo[i].nomeCampo, "codEstacao") == 0)
                codEstacao = busca->campo[i].valorInt;
        }
    }

    // Retorno
    return codEstacao;
}

/**
 * @brief Analisa se as propriedades de um registro combinam com TODOS os critérios da busca.
 * @param registro O registro lido em disco a ser testado.
 * @param busca O ponteiro com os parâmetros de filtro.
 * @return int 1 se passar em todos os filtros; 0 se não atender a algum critério.
 */
int utils_compararRegistroComFiltros(Registro *registro, Busca *busca) {

    // --- Loop para todos os filtros ---
    for (int i = 0; i < busca->mCampos; i++) {

        // Campo atual
        Campo *campo = &busca->campo[i];

        // Se atende todos os critérios
        int atendeCriterio = 0;

        // Campos inteiros
        if (campo->isString == 0) {

            // Vamos pegar o valor do campo
            int valorRegistro;

            if      (strcmp(campo->nomeCampo, "codEstacao") == 0)      valorRegistro = registro->codEstacao;
            else if (strcmp(campo->nomeCampo, "codLinha") == 0)        valorRegistro = registro->codLinha;
            else if (strcmp(campo->nomeCampo, "codProxEstacao") == 0)  valorRegistro = registro->codProxEstacao;
            else if (strcmp(campo->nomeCampo, "distProxEstacao") == 0) valorRegistro = registro->distProxEstacao;
            else if (strcmp(campo->nomeCampo, "codLinhaIntegra") == 0) valorRegistro = registro->codLinhaIntegra;
            else if (strcmp(campo->nomeCampo, "codEstIntegra") == 0)   valorRegistro = registro->codEstIntegra;

            // Verificamos se atende ao critério
            if (valorRegistro == campo->valorInt) {
                atendeCriterio = 1;
            }
        }
        
        // Campos string 
        else if (campo->isString == 1) {

            // Vamos pegar o valor do campo
            char *valorRegistro;
            int tamValorRegistro;

            if (strcmp(campo->nomeCampo, "nomeEstacao") == 0) {
                valorRegistro = registro->nomeEstacao;
                tamValorRegistro = registro->tamNomeEstacao;
            } 
            else if (strcmp(campo->nomeCampo, "nomeLinha") == 0) {
                valorRegistro = registro->nomeLinha;
                tamValorRegistro = registro->tamNomeLinha;
            }

            // Se filtro for NULO -> tamanho no registro = 0 (verificamos se bate)
            if (strcmp(campo->valorString, "NULO") == 0) {
                if (tamValorRegistro == 0) { // Campo é NULO
                    atendeCriterio = 1;
                }
            } 
            // Se filtro não for nulo, fazemos comparação normal de strings
            else {
                if (strcmp(valorRegistro, campo->valorString) == 0) {
                    atendeCriterio = 1;
                }
            }
        }
        
        // Se não atender a algum dos filtros
        if (!atendeCriterio) return 0; 
    }

    // Se passou em todos os filtros
    return 1; 
}


/* ========================================================================== *
 * ATUALIZAÇÃO DE REGISTROS (UPDATE)                                          *
 * ========================================================================== */

/**
 * @brief Sobrescreve campos de um registro físico específico do arquivo.
 * @param busca A estrutura preenchida com as chaves e valores a serem inseridos.
 * @param arquivoBin O arquivo de dados em aberto.
 * @param offsetAtual O cursor do laço exterior (apontando para o byte após o fim deste registro).
 */
void utils_atualizarRegistroComFiltros(Busca busca, FILE *arquivoBin, int offsetAtual) {

    // Calculamos o offset do começo do registro
    int offsetRegistro = offsetAtual - TAM_REGISTRO;

    // Lemos o registro
    Registro registro;
    fseek(arquivoBin, offsetRegistro, SEEK_SET);
    registro_lerRegistroBin(arquivoBin, &registro);

    // Campos com valores atualizados
    Campo *campo = busca.campo;

    // -- Loop pelos campos ---
    for (int j = 0; j < busca.mCampos; j++) {

        // Agora, vamos atualizar o registro conforme os campos
        if      (strcmp(campo[j].nomeCampo, "codEstacao") == 0)      registro.codEstacao = campo[j].valorInt;
        else if (strcmp(campo[j].nomeCampo, "codLinha") == 0)        registro.codLinha = campo[j].valorInt;
        else if (strcmp(campo[j].nomeCampo, "codProxEstacao") == 0)  registro.codProxEstacao = campo[j].valorInt;
        else if (strcmp(campo[j].nomeCampo, "distProxEstacao") == 0) registro.distProxEstacao = campo[j].valorInt; 
        else if (strcmp(campo[j].nomeCampo, "codLinhaIntegra") == 0) registro.codLinhaIntegra = campo[j].valorInt;
        else if (strcmp(campo[j].nomeCampo, "codEstIntegra") == 0)   registro.codEstIntegra = campo[j].valorInt;
        
        else if (strcmp(campo[j].nomeCampo, "nomeEstacao") == 0) {
            strcpy(registro.nomeEstacao, campo[j].valorString);           
            registro.tamNomeEstacao = strlen(registro.nomeEstacao);
        }

        else if (strcmp(campo[j].nomeCampo, "nomeLinha") == 0) {
            strcpy(registro.nomeLinha, campo[j].valorString);           
            registro.tamNomeLinha = strlen(registro.nomeLinha);
        }
    }

    // Por fim, é só escrever o registro :D
    fseek(arquivoBin, offsetRegistro, SEEK_SET);
    registro_escreverRegistroBin(arquivoBin, &registro);
}