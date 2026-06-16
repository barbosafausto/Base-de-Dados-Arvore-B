#include "arvoreb.h"

/* ========================================================================== *
 * FUNÇÕES DE GERENCIAMENTO DE CABEÇALHO                                      *
 * ========================================================================== */

/**
 * @brief Inicializa o registro de cabeçalho com os valores padrão de uma árvore vazia.
 * * @param cabecalhoAB Ponteiro para a estrutura de cabeçalho na memória principal.
 */
void arvoreb_initCabecalho(CabecalhoAB *cabecalhoAB) {
    cabecalhoAB->status = '0';            // '0' indica arquivo inconsistente durante o uso
    cabecalhoAB->noRaiz = -1;             // -1 indica que a árvore-B está inicialmente vazia
    cabecalhoAB->topo = -1;               // -1 indica ausência de registros logicamente removidos
    cabecalhoAB->proxRRN = 0;             // RRN 0 será o primeiro a ser utilizado para um novo nó
    cabecalhoAB->nroNos = 0;              // Contador inicial de nós (páginas) na árvore
}

/**
 * @brief Lê o registro de cabeçalho do arquivo de índice binário para a memória principal.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário da árvore-B.
 * @param cabecalhoAB Ponteiro para a estrutura de cabeçalho que receberá os dados.
 */
void arvoreb_lerCabecalhoBin(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB) {
    fread(&cabecalhoAB->status, sizeof(char), 1, arquivoIndiceBin);
    fread(&cabecalhoAB->noRaiz, sizeof(int), 1, arquivoIndiceBin);
    fread(&cabecalhoAB->topo, sizeof(int), 1, arquivoIndiceBin);
    fread(&cabecalhoAB->proxRRN, sizeof(int), 1, arquivoIndiceBin);
    fread(&cabecalhoAB->nroNos, sizeof(int), 1, arquivoIndiceBin);
}

/**
 * @brief Escreve os dados do registro de cabeçalho da RAM para o arquivo binário.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário da árvore-B.
 * @param cabecalhoAB Ponteiro para a estrutura de cabeçalho contendo os dados atualizados.
 */
void arvoreb_escreverCabecalhoBin(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB) {
    fseek(arquivoIndiceBin, 0, SEEK_SET);
    fwrite(&cabecalhoAB->status,          sizeof(char), 1, arquivoIndiceBin);
    fwrite(&cabecalhoAB->noRaiz,          sizeof(int), 1, arquivoIndiceBin);
    fwrite(&cabecalhoAB->topo,            sizeof(int), 1, arquivoIndiceBin);
    fwrite(&cabecalhoAB->proxRRN,         sizeof(int), 1, arquivoIndiceBin);
    fwrite(&cabecalhoAB->nroNos,         sizeof(int), 1, arquivoIndiceBin);
}

/**
 * @brief Gerencia a consistência e a gravação do status do arquivo de índice.
 * * @param cabecalhoAB Ponteiro para a estrutura do cabeçalho.
 * @param arquivoIndiceBin Ponteiro para o arquivo binário.
 * @param escreverConsistente Flag: 1 para definir arquivo como consistente e fechar/gravar.
 * @param abertoParaLeitura Flag: 1 se o arquivo foi aberto apenas no modo de leitura ("rb").
 * @return int 1 em caso de sucesso na verificação/escrita, 0 em caso de falha (arquivo inconsistente).
 */
int arvoreb_gerenciaCabecalho(CabecalhoAB *cabecalhoAB, FILE *arquivoIndiceBin, int escreverConsistente, int abertoParaLeitura) {
    
    // Escrita consistente
    if (escreverConsistente == 1) {
        cabecalhoAB->status = '1'; 
        fseek(arquivoIndiceBin, 0, SEEK_SET);
        arvoreb_escreverCabecalhoBin(arquivoIndiceBin, cabecalhoAB);
        return 1;
    }
    
    // Verificação de consistência
    if (cabecalhoAB->status == '0') { 
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoIndiceBin);
        return 0;
    }

    // Escrita inconsistente
    if (abertoParaLeitura == 0) {
        cabecalhoAB->status = '0';
        fseek(arquivoIndiceBin, 0, SEEK_SET);
        fwrite(&cabecalhoAB->status, sizeof(char), 1, arquivoIndiceBin); 
    }

    return 1;
}

/* ========================================================================== *
 * FUNÇÕES DE MANIPULAÇÃO DOS NÓS                                             *
 * ========================================================================== */

/**
 * @brief Lê uma página (nó) específica da árvore-B diretamente do disco.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário da árvore-B.
 * @param RRN RRN indicando a posição do nó no disco.
 * @return NO Estrutura do nó preenchida com os dados lidos do arquivo.
 */
NO arvoreb_lerNoBin(FILE *arquivoIndiceBin, int RRN) {
    
    // Struct que guardará as informações lidas
    NO node; 

    // Posicionando a leitora no offset correto
    fseek(arquivoIndiceBin, TAM_CABECALHOAB + RRN * TAM_NO, SEEK_SET);

    // --- Leitura ---
    fread(&node.removido, sizeof(char), 1, arquivoIndiceBin);   
    fread(&node.proximo, sizeof(int), 1, arquivoIndiceBin);
    fread(&node.tipoNo, sizeof(int), 1, arquivoIndiceBin);
    fread(&node.nroChaves, sizeof(int), 1, arquivoIndiceBin);

    for (int i = 0; i < ORDEM_ARVORE-1; i++) {
        fread(&node.estacao[i].chave, sizeof(int), 1, arquivoIndiceBin);
        fread(&node.estacao[i].Pr, sizeof(int), 1, arquivoIndiceBin);
    }
    
    fread(&node.P1, sizeof(int), 1, arquivoIndiceBin);

    for (int i = 0; i < ORDEM_ARVORE-1; i++) 
        fread(&node.estacao[i].P2, sizeof(int), 1, arquivoIndiceBin);

    // Retorno do nó preenchido
    return node;
}

/**
 * @brief Instancia e inicializa um novo nó (página) da árvore-B na memória.
 * * @param cabecalhoAB Ponteiro para o registro de cabeçalho para atualização de métricas.
 * @param tipoNo Tipo do nó (-1: folha, 0: raiz, 1: intermediário).
 * @param P1 RRN do filho mais à esquerda (ou -1).
 * @param estacao Vetor de estações que comporão inicialmente o nó.
 * @return NO Nova estrutura de nó pronta para ser inserida ou gravada.
 */
NO arvoreb_criarNoBin(CabecalhoAB *cabecalhoAB, int tipoNo, int P1, Estacao *estacao) {

    NO node;

    // --- Informações iniciais ---
    node.removido = '0';      
    node.proximo = -1;      
    node.tipoNo = tipoNo;

    // O único nó que pode ter apenas uma chave é a raiz
    if (tipoNo == 0 || (tipoNo == -1 && cabecalhoAB->nroNos == 0))
        node.nroChaves = 1;
    else 
        node.nroChaves = OCUPACAO_MINIMA;

    // --- Preenchimento do nó ---
    node.P1 = P1; 
    
    for (int i = 0; i < node.nroChaves; i++) 
        node.estacao[i] = estacao[i];

    Estacao estacaoVazia = {-1, -1, -1};
    for (int i = node.nroChaves; i < ORDEM_ARVORE-1; i++) 
        node.estacao[i] = estacaoVazia;

    // --- Atualização do cabeçalho ---
    cabecalhoAB->nroNos++;
    cabecalhoAB->proxRRN++;

    // Retorno do nó preenchido
    return node;
}

/**
 * @brief Grava a estrutura de um nó (página) da RAM para a memória secundária (disco).
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário.
 * @param node Ponteiro para a estrutura contendo os dados a serem gravados.
 * @param RRN  RRN alvo da gravação.
 */
void arvoreb_escreverNoBin(FILE *arquivoIndiceBin, NO *node, int RRN) {

    // Posicionamento da leitora
    fseek(arquivoIndiceBin, TAM_CABECALHOAB + RRN * TAM_NO, SEEK_SET);

    // --- Escrita do nó ---
    fwrite(&node->removido, sizeof(char), 1, arquivoIndiceBin);
    fwrite(&node->proximo, sizeof(int), 1, arquivoIndiceBin);
    fwrite(&node->tipoNo, sizeof(int), 1, arquivoIndiceBin);
    fwrite(&node->nroChaves, sizeof(int), 1, arquivoIndiceBin);

    for (int i = 0; i < ORDEM_ARVORE-1; i++) {
        fwrite(&node->estacao[i].chave, sizeof(int), 1, arquivoIndiceBin);
        fwrite(&node->estacao[i].Pr, sizeof(int), 1, arquivoIndiceBin);
    }

    fwrite(&node->P1, sizeof(int), 1, arquivoIndiceBin);
    for (int i = 0; i < ORDEM_ARVORE-1; i++)
        fwrite(&node->estacao[i].P2, sizeof(int), 1, arquivoIndiceBin);
}

/**
 * @brief Obtém um RRN disponível, dando prioridade à pilha de nós removidos.
 * Atualiza o cabeçalho (topo, proxRRN e nroNos) mas não o grava (o caller decide quando gravar).
 * *
 * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
 * @param cabecalhoAB Ponteiro para o cabeçalho da árvore-B em RAM.
 * *
 * @return RRN disponível para criação ou reutilização de um nó.
 */
int arvoreb_obterRRNNovoNo(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB) {
    int rrnNovo;

    // Prioridade 1: Reutiliza o topo da pilha de nós logicamente removidos
    if (cabecalhoAB->topo != -1) {
        rrnNovo = cabecalhoAB->topo;

        // Lê o nó removido para descobrir quem é o próximo da pilha
        NO noRemovido = arvoreb_lerNoBin(arquivoIndiceBin, rrnNovo);
        
        // O topo passa a ser o RRN armazenado no campo 'proximo'
        cabecalhoAB->topo = noRemovido.proximo;
        cabecalhoAB->nroNos++; // Reativando um nó
    } 
    // Prioridade 2: Pega um novo RRN do fim do arquivo
    else {
        rrnNovo = cabecalhoAB->proxRRN;
        cabecalhoAB->proxRRN++;
        cabecalhoAB->nroNos++;
    }

    return rrnNovo;
}

/**
 * @brief Monta a estrutura do nó estritamente em memória primária.
 * Isola a responsabilidade de formatação sem afetar as métricas do cabeçalho.
 * *
 * @param tipoNo Tipo do nó criado (raiz, intermediário ou folha).
 * @param P1 Primeiro ponteiro filho do nó.
 * @param estacao Vetor contendo as chaves que serão copiadas para o nó.
 * @param nroChaves Quantidade de chaves válidas presentes no vetor estacao.
 * *
 * @return Nó da árvore-B montado em memória.
 */
NO arvoreb_montarNo(int tipoNo, int P1, Estacao *estacao, int nroChaves) {
    NO no;

    no.removido = '0';      
    no.proximo = -1;      
    no.tipoNo = tipoNo;
    no.nroChaves = nroChaves;
    no.P1 = P1; 
    
    // Cópia das chaves passadas
    for (int i = 0; i < nroChaves; i++) 
        no.estacao[i] = estacao[i];

    // Padding nulo para os espaços não utilizados
    Estacao estacaoVazia = {-1, -1, -1};
    for (int i = nroChaves; i < ORDEM_ARVORE-1; i++) 
        no.estacao[i] = estacaoVazia;

    return no;
}

/* ========================================================================== *
 * FUNCIONALIDADES DE BUSCA                                                   *
 * ========================================================================== */

/**
 * @brief Realiza busca binária dentro de um nó para localizar uma chave ou determinar a subárvore correta.
 * * @param node Ponteiro para o nó onde a busca binária será executada.
 * @param chave Chave de busca (codEstacao) desejada.
 * @return Estacao* Retorna o endereço da estação exata (se encontrada) ou a estação que contém o ponteiro P2 do caminho de descida.
 */
Estacao* arvoreb_buscaBinaria(NO *node, int chave) {
    
    // Variável de retorno
    Estacao *retorno = NULL; 

    // Limites da busca
    int ini = 0, fim = node->nroChaves-1, mid;

    // Estação atual da busca
    Estacao *estacao;
    
    // --- Busca Binária ---
    while (ini <= fim) {
        mid = (ini+fim) >> 1; 
        estacao = &node->estacao[mid];

        if (chave == estacao->chave)
            return estacao;

        if (chave > estacao->chave) {
            retorno = estacao;
            ini = mid+1;
        }
        else {
            fim = mid-1;
        }
    }

    // Retorno
    return retorno;
}

/**
 * @brief Função recursiva para percorrer a árvore-B em busca de uma chave.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário.
 * @param cabecalhoAB Ponteiro para o cabeçalho.
 * @param nodeRRN RRN do nó atual na recursão.
 * @param chave Chave de busca solicitada.
 * @return int Retorna a referência (Pr/byte offset) do registro de dados, ou -1 se não for encontrado.
 */
int arvoreb_buscarRecursivo(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int nodeRRN, int chave) {

    // Caso base
    if (nodeRRN == -1)
        return -1;

    // Leitura do nó atual
    NO node = arvoreb_lerNoBin(arquivoIndiceBin, nodeRRN);

    // Caso especial: verificamos se devemos descer pelo filho mais à esquerda do nó
    if (chave < node.estacao[0].chave)
        return arvoreb_buscarRecursivo(arquivoIndiceBin, cabecalhoAB, node.P1, chave);

    // Senão, fazer a busca binária
    Estacao *retorno = arvoreb_buscaBinaria(&node, chave);

    // Se encontramos a chave, retornamos seu offset no arquivo de adados
    if (retorno->chave == chave) 
        return retorno->Pr;

    // Senão, continuamos a busca
    return arvoreb_buscarRecursivo(arquivoIndiceBin, cabecalhoAB, retorno->P2, chave);
}

/**
 * @brief Função que inicializa a busca na árvore-B a partir do nó raiz.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário.
 * @param cabecalhoAB Ponteiro para o cabeçalho contendo o RRN da raiz.
 * @param chave Chave de busca.
 * @return int Retorna o offset do arquivo de dados ou -1 em caso de falha.
 */
int arvoreb_buscar(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int chave) {
    return arvoreb_buscarRecursivo(arquivoIndiceBin, cabecalhoAB, cabecalhoAB->noRaiz, chave);
}

/* ========================================================================== *
 * FUNCIONALIDADES DE INSERÇÃO                                                *
 * ========================================================================== */

/**
 * @brief Insere ordenadamente uma nova estação em um vetor de estações pré-ordenado.
 * * @param estacoes Vetor contendo as chaves atuais (pode ser o próprio nó ou o vetor temporário de split).
 * @param estacaoParaInserir Ponteiro para a estação a ser acomodada.
 * @param nroChaves Quantidade atual de chaves já presentes no vetor.
 */
void arvoreb_ordenaNo(Estacao *estacoes, Estacao *estacaoParaInserir, int nroChaves) {

    for (int i = nroChaves; i >= 0; i--) {
        
        // É parecido com um Insertion Sort
        // Vamos shiftando até achar o ponto de colocar a estação atual
        if (i == 0 || estacoes[i-1].chave < estacaoParaInserir->chave) {
            estacoes[i] = *estacaoParaInserir; 
            break;
        }
        
        // Shift para a direita
        else {
            estacoes[i] = estacoes[i-1];
        }
    }
}

/**
 * @brief Executa o particionamento (split) de uma página em overflow e gerencia a promoção de chave.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário.
 * @param cabecalhoAB Ponteiro para o cabeçalho da árvore-B.
 * @param estacaoParaInserir Ponteiro para a estação que causou o overflow.
 * @param node Ponteiro para a página original que sofreu overflow.
 * @param nodeRRN RRN da página original.
 * @return Estacao Retorna a chave mediana promovida contendo o RRN da nova página acoplado ao seu ponteiro P2.
 */
Estacao arvoreb_split(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, Estacao *estacaoParaInserir, NO *node, int nodeRRN) {

    // --- Etapa 1: Preparação do Vetor com Todas as Estações (Ordenação) ---
    Estacao estacoes[ORDEM_ARVORE]; 

    // População do vetor com o nó
    for (int i = 0; i < ORDEM_ARVORE-1; i++)
        estacoes[i] = node->estacao[i];

    // Encontrando um lugar para a nova estação no vetor
    arvoreb_ordenaNo(estacoes, estacaoParaInserir, ORDEM_ARVORE-1);

    // --- Etapa 2: Definição da Mediana e Distribuição de Chaves ---

    // A chave promovida é a própria mediana 
    Estacao estacaoPromovida = estacoes[ORDEM_ARVORE/2]; 

    // RRN do novo nó (considerando possíveis remoções)
    int RRNNovoNO = arvoreb_obterRRNNovoNo(arquivoIndiceBin, cabecalhoAB);

    // População do novo nó
    NO novoNo; 
    for (int i = 0; i < OCUPACAO_MINIMA; i++)
        novoNo.estacao[i] = estacoes[ORDEM_ARVORE/2 + 1 + i];
    novoNo.P1 = estacaoPromovida.P2;

    // A estação promovida (mediana) tem o novo nó como filho direito
    estacaoPromovida.P2 = RRNNovoNO;

    // Preenchimento do nó original com o vetor de estacoes **após a ordenação**
    for (int i = 0; i < ORDEM_ARVORE/2; i++)
        node->estacao[i] = estacoes[i];

    // Preenchimento do espaço restante (vazio)
    Estacao estacaoVazia = {-1, -1, -1};
    for (int i = ORDEM_ARVORE/2; i < ORDEM_ARVORE-1; i++)
        node->estacao[i] = estacaoVazia;

    // Númmero de chaves do original
    node->nroChaves = ORDEM_ARVORE / 2;

    // --- Etapa 3: Tratamento e Hierarquia de Tipos de Nó ---
    
    if (node->P1 == -1) {
        node->tipoNo = -1; // folha
    } else {
        node->tipoNo = 1;  // intermediário
    }

    if (novoNo.P1 == -1) {
        novoNo.tipoNo = -1;
    } else {
        novoNo.tipoNo = 1;
    }

    arvoreb_escreverNoBin(arquivoIndiceBin, node, nodeRRN);

    // --- Etapa 4: Criação e Gravação Persistente ---

    novoNo = arvoreb_montarNo(novoNo.tipoNo, novoNo.P1, novoNo.estacao, OCUPACAO_MINIMA);
    arvoreb_escreverNoBin(arquivoIndiceBin, &novoNo, RRNNovoNO);
    
    // Se o nó atual é a raiz, então uma nova raiz precisa ser criada
    if (nodeRRN == cabecalhoAB->noRaiz) {

        // RRN da nova raiz
        int RRNNovaRaiz = arvoreb_obterRRNNovoNo(arquivoIndiceBin, cabecalhoAB);
        
        // Atualização do cabeçalho
        cabecalhoAB->noRaiz = RRNNovaRaiz; 
        
        // Montagem do nó e escrita
        NO novaRaiz = arvoreb_montarNo(0, nodeRRN, &estacaoPromovida, 1);
        arvoreb_escreverNoBin(arquivoIndiceBin, &novaRaiz, RRNNovaRaiz);
    } 

    // Retornamos a estação promovida
    return estacaoPromovida; 
}

/**
 * @brief Fluxo recursivo principal de inserção de chave ou tratamento de promoções.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário.
 * @param cabecalhoAB Ponteiro para o cabeçalho.
 * @param nodeRRN RRN da página alvo desta etapa da recursão.
 * @param estacaoParaInserir Estação contendo a chave candidata a inserção.
 * @return Estacao Retorna uma estação formatada se houver  promoção (split interno) ou chave nula se a operação finalizou no nível inferior.
 */
Estacao arvoreb_inserirRecursivo(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int nodeRRN, Estacao estacaoParaInserir) {
    
    // Nó atual
    NO node = arvoreb_lerNoBin(arquivoIndiceBin, nodeRRN);

    // --- Recursão somente em nós intermediários ou na raiz, se nroNos > 1
    if (node.tipoNo != -1 && cabecalhoAB->nroNos > 1) {
        
        // Caso especial: vemos se precisamos descer na subárvore mais à esquerda do nó
        if (estacaoParaInserir.chave < node.estacao[0].chave)
            estacaoParaInserir = arvoreb_inserirRecursivo(arquivoIndiceBin, cabecalhoAB, node.P1, estacaoParaInserir);

        // Caso padrão: busca binária (encontra a chave ou desce mais)
        else {
            Estacao *retorno = arvoreb_buscaBinaria(&node, estacaoParaInserir.chave);

            if (retorno->chave == estacaoParaInserir.chave) {
                estacaoParaInserir.chave = -1; // Se encontrar, chave = -1 (não fazemos inserções no retorno)
                return estacaoParaInserir;
            }
            estacaoParaInserir = arvoreb_inserirRecursivo(arquivoIndiceBin, cabecalhoAB, retorno->P2, estacaoParaInserir);
        }
    }

    // --- Caso base: se chave < 0, não há nada para inserir ---
    if (estacaoParaInserir.chave < 0)
        return estacaoParaInserir;

    // --- Inserindo com espaço ---
    if (node.nroChaves < ORDEM_ARVORE-1) {
        arvoreb_ordenaNo(node.estacao, &estacaoParaInserir, node.nroChaves); 
        node.nroChaves++;

        estacaoParaInserir.chave = -1; 
        arvoreb_escreverNoBin(arquivoIndiceBin, &node, nodeRRN); 
    }
    
    // --- Inserindo sem espaço: split ---
    else {
        estacaoParaInserir = arvoreb_split(arquivoIndiceBin, cabecalhoAB, &estacaoParaInserir, &node, nodeRRN);
    }

    // Retorno 
    return estacaoParaInserir; 
}

/**
 * @brief Função principal para solicitar uma inserção na estrutura da árvore-B.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
 * @param cabecalhoAB Ponteiro para o cabeçalho em RAM.
 * @param estacaoParaInserir Estação (chave + byte offset) inicial lida do arquivo de dados que desencadeia a operação.
 */
void arvoreb_inserir(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, Estacao estacaoParaInserir) {

    NO node;

    // --- Caso em que não há raiz na árvore ---
    if (cabecalhoAB->noRaiz == -1) {
        
        // Atualização do cabeçalho
        cabecalhoAB->noRaiz = cabecalhoAB->proxRRN;
        
        // Criação da raiz
        node = arvoreb_criarNoBin(cabecalhoAB, -1, -1, &estacaoParaInserir);
        arvoreb_escreverNoBin(arquivoIndiceBin, &node, cabecalhoAB->noRaiz);
        return;
    }
    
    // --- Caso em que há raiz: começamos a recursão
    arvoreb_inserirRecursivo(arquivoIndiceBin, cabecalhoAB, cabecalhoAB->noRaiz, estacaoParaInserir);
}


/* ========================================================================== *
 * FUNCIONALIDADES DE REMOÇÃO/AUXILIARES                                      *
 * ========================================================================== */

/**
  * @brief Encontra a posição de uma chave dentro de um nó da árvore-B.
  * *
  * @param node Ponteiro para o nó da árvore-B em RAM.
  * @param chave Chave procurada dentro do nó.
  * *
  * @return Posição da chave no nó, ou posição do filho onde a busca deve continuar (para descer na árvore).
*/
int arvoreb_encontrarPosicao(NO *node, int chave) {
    int i = 0;
    while (i < node->nroChaves && chave > node->estacao[i].chave) {
        i++;
    }
    return i;
}

/**
  * @brief Recupera (get) o RRN de um filho de acordo com sua posição no nó.
  * *
  * @param node Ponteiro para o nó da árvore-B em RAM.
  * @param pos Posição do filho a ser recuperado.
  * *
  * @return RRN do filho correspondente, ou -1 caso a posição seja inválida.
*/
int arvoreb_getFilho(NO *node, int pos) {
    if (pos < 0 || pos > ORDEM_ARVORE - 1) return -1;

    if (pos == 0) return node->P1;
    return node->estacao[pos - 1].P2;
}

/**
  * @brief Atualiza (set) o RRN de um filho de acordo com sua posição no nó.
  * *
  * @param node Ponteiro para o nó da árvore-B em RAM.
  * @param pos Posição do filho a ser atualizado.
  * @param rrn Novo RRN que será armazenado como ponteiro filho.
*/
void arvoreb_setFilho(NO *node, int pos, int rrn) {
    if (pos < 0 || pos > ORDEM_ARVORE - 1) return;

    if (pos == 0) {
        node->P1 = rrn;
    } else {
        node->estacao[pos - 1].P2 = rrn;
    }
}

/**
  * @brief Remove uma chave de um nó da árvore-B. Desloca as chaves e mantém o nó ordenado.
  * *
  * @param node Ponteiro para o nó da árvore-B em RAM.
  * @param pos Posição da chave que será removida do nó.
*/
void arvoreb_removerChaveDoNo(NO *node, int pos) {
    // Desloca as chaves, referências e ponteiros para a esquerda a partir de 'pos'
    for (int i = pos; i < node->nroChaves - 1; i++) {
        node->estacao[i] = node->estacao[i + 1];
    }
    
    // Limpa a última posição com valores nulos (-1)
    Estacao vazia = {-1, -1, -1};
    node->estacao[node->nroChaves - 1] = vazia;
    node->nroChaves--;
}

/**
  * @brief Busca a sucessora imediata em folha dentro da subárvore direita (menor da subárvore direita).
  * *
  * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
  * @param rrnSubarvoreDireita RRN da raiz da subárvore direita.
  * *
  * @return Estação sucessora imediata encontrada.
*/
Estacao arvoreb_buscarSucessora(FILE *arquivoIndiceBin, int rrnSubarvoreDireita) {
    NO atual = arvoreb_lerNoBin(arquivoIndiceBin, rrnSubarvoreDireita);
    
    // A sucessora imediata é a primeira chave da folha mais à esquerda da subárvore direita
    while (atual.tipoNo != -1) {
        atual = arvoreb_lerNoBin(arquivoIndiceBin, atual.P1);
    }
    
    return atual.estacao[0];
}

/**
  * @brief Empilha um nó removido na pilha de páginas removidas da árvore-B.
  * *
  * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
  * @param cabecalhoAB Ponteiro para o cabeçalho da árvore-B em RAM.
  * @param rrnRemovido RRN do nó que será marcado como removido e empilhado.
*/
void arvoreb_empilharNoRemovido(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int rrnRemovido) {
    NO removido = arvoreb_lerNoBin(arquivoIndiceBin, rrnRemovido);
    
    // Adiciona na pilha: o próximo do nó removido aponta para o topo atual
    removido.removido = '1';
    removido.proximo = cabecalhoAB->topo;
    
    // Escreve a alteração no nó e atualiza o cabeçalho
    arvoreb_escreverNoBin(arquivoIndiceBin, &removido, rrnRemovido);
    
    cabecalhoAB->topo = rrnRemovido;
    cabecalhoAB->nroNos--;
}

/* -------------------------------------------------------------------------- *
 * REDISTRIBUIÇÃO E CONCATENAÇÃO (Underflow)                                  *
 * -------------------------------------------------------------------------- */
/**
 * @brief Tenta corrigir underflow redistribuindo chaves com o irmão direito.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
 * @param rrnPai RRN da página pai.
 * @param pai Ponteiro para o nó pai em RAM.
 * @param posFilho Posição lógica do filho em underflow dentro do pai.
 * @param rrnFilho RRN do filho em underflow.
 * @param filho Ponteiro para o nó filho em underflow.
 * @param rrnDir RRN do irmão direito.
 * * @return 1 se a redistribuição foi realizada, ou 0 caso contrário.
 */
int arvoreb_redistribuirDireita(FILE *arquivoIndiceBin, int rrnPai, NO *pai, int posFilho, int rrnFilho, NO *filho, int rrnDir) {
    if (rrnDir == -1) return 0; // Sem irmão direito

    NO irmaoDir = arvoreb_lerNoBin(arquivoIndiceBin, rrnDir);

    if (irmaoDir.nroChaves <= OCUPACAO_MINIMA) return 0; // Só refistribui se tiver mais que a ocupação mínima

    Estacao vazia = {-1, -1, -1};

    // Caso 1: irmão direito com duas chaves (uma chave transferida).
    if (irmaoDir.nroChaves == 2) {
        
        // 1. A chave separadora do pai desce para o final do filho
        filho->estacao[filho->nroChaves].chave = pai->estacao[posFilho].chave;
        filho->estacao[filho->nroChaves].Pr = pai->estacao[posFilho].Pr;
        filho->estacao[filho->nroChaves].P2 = irmaoDir.P1;
        filho->nroChaves++;

        // 2. Primeira chave do irmão direito sobe para o pai
        pai->estacao[posFilho].chave = irmaoDir.estacao[0].chave;
        pai->estacao[posFilho].Pr = irmaoDir.estacao[0].Pr;

        // 3. Antigo P2 da primeira chave (irmão direito) vira o novo P1
        irmaoDir.P1 = irmaoDir.estacao[0].P2;
        arvoreb_removerChaveDoNo(&irmaoDir, 0);
    }

    // Caso 2: irmão direito tem 3 chaves, redistribuição deve ficar uniforme
    else if (irmaoDir.nroChaves == 3) {

        // Salvar chaves antes de reorganizar
        Estacao r0 = irmaoDir.estacao[0];
        Estacao r1 = irmaoDir.estacao[1];
        Estacao r2 = irmaoDir.estacao[2];

        // 1. A chave separadora do pai desce para o final do filho
        filho->estacao[filho->nroChaves].chave = pai->estacao[posFilho].chave;
        filho->estacao[filho->nroChaves].Pr = pai->estacao[posFilho].Pr;
        filho->estacao[filho->nroChaves].P2 = irmaoDir.P1;
        filho->nroChaves++;

        // 2. A primeira chave do irmão direito também vai para o filho
        filho->estacao[filho->nroChaves] = r0;
        filho->nroChaves++;

        // 3. A segunda chave do irmão direito sobe para o pai
        pai->estacao[posFilho].chave = r1.chave;
        pai->estacao[posFilho].Pr = r1.Pr;

        // 4. O irmão direito fica apenas com a terceira chave
        irmaoDir.P1 = r1.P2;
        irmaoDir.estacao[0] = r2;
        irmaoDir.estacao[1] = vazia;
        irmaoDir.estacao[2] = vazia;
        irmaoDir.nroChaves = 1;
    }

    arvoreb_escreverNoBin(arquivoIndiceBin, filho, rrnFilho);
    arvoreb_escreverNoBin(arquivoIndiceBin, &irmaoDir, rrnDir);
    arvoreb_escreverNoBin(arquivoIndiceBin, pai, rrnPai);

    return 1;
}
*/

//FUNÇÃO DE REDISTRIBUIÇÃO GENERALIZADA ABAIXO

/**
 * @brief Executa empréstimo de UMA chave do irmão direito para suprir um filho em underflow.
 * A rotação é generalizada para qualquer ordem de Árvore-B através da chave separadora no pai.
 * @param arquivoIndiceBin Arquivo binário.
 * @param rrnPai RRN do pai.
 * @param pai Ponteiro do pai na RAM.
 * @param posFilho Índice do filho em underflow.
 * @param rrnFilho RRN do nó em underflow.
 * @param filho Ponteiro do filho na RAM.
 * @param rrnDir RRN do irmão direito (ou -1 se inexistente).
 * @return int 1 se o empréstimo foi executado; 0 se irmão não tem saldo para emprestar.
 */
int arvoreb_redistribuirDireita(FILE *arquivoIndiceBin, int rrnPai, NO *pai, int posFilho, int rrnFilho, NO *filho, int rrnDir) {
    if (rrnDir == -1) return 0;

    NO irmaoDir = arvoreb_lerNoBin(arquivoIndiceBin, rrnDir);

    // Se o irmão tem o mínimo, não pode emprestar, caso contrário causaria underflow em cadeia.
    if (irmaoDir.nroChaves <= OCUPACAO_MINIMA) return 0; 

    // 1. A chave separadora do pai desce para a última posição disponível do filho
    filho->estacao[filho->nroChaves].chave = pai->estacao[posFilho].chave;
    filho->estacao[filho->nroChaves].Pr = pai->estacao[posFilho].Pr;
    
    // O filho adopta como P2 a antiga âncora (P1) do irmão direito
    filho->estacao[filho->nroChaves].P2 = irmaoDir.P1; 
    filho->nroChaves++;

    // 2. A primeira chave do irmão direito sobe para substituir o separador no pai
    pai->estacao[posFilho].chave = irmaoDir.estacao[0].chave;
    pai->estacao[posFilho].Pr = irmaoDir.estacao[0].Pr;

    // 3. O antigo P2 da chave que acabou de subir torna-se a nova âncora (P1) do irmão direito
    irmaoDir.P1 = irmaoDir.estacao[0].P2;

    // 4. Remove a primeira chave do irmão direito 
    // (A função arvoreb_removerChaveDoNo já empurra todas as restantes para a esquerda de forma automática!)
    arvoreb_removerChaveDoNo(&irmaoDir, 0);

    // Salva as alterações sincronizadas no ficheiro binário
    arvoreb_escreverNoBin(arquivoIndiceBin, filho, rrnFilho);
    arvoreb_escreverNoBin(arquivoIndiceBin, &irmaoDir, rrnDir);
    arvoreb_escreverNoBin(arquivoIndiceBin, pai, rrnPai);

    return 1;
}


/**
 * @brief Tenta corrigir underflow redistribuindo chaves com o irmão esquerdo.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
 * @param rrnPai RRN da página pai.
 * @param pai Ponteiro para o nó pai em RAM.
 * @param posFilho Posição lógica do filho em underflow dentro do pai.
 * @param rrnFilho RRN do filho em underflow.
 * @param filho Ponteiro para o nó filho em underflow.
 * @param rrnEsq RRN do irmão esquerdo.
 * * @return 1 se a redistribuição foi realizada, ou 0 caso contrário.
 */
int arvoreb_redistribuirEsquerda(FILE *arquivoIndiceBin, int rrnPai, NO *pai, int posFilho, int rrnFilho, NO *filho, int rrnEsq) {
    if (rrnEsq == -1) return 0;
    
    NO irmaoEsq = arvoreb_lerNoBin(arquivoIndiceBin, rrnEsq);
    if (irmaoEsq.nroChaves <= OCUPACAO_MINIMA) return 0; // Não pode emprestar

    // 1. Abre espaço no início do filho empurrando tudo para a direita
    for (int i = filho->nroChaves; i > 0; i--) {
        filho->estacao[i] = filho->estacao[i - 1];
    }
    filho->estacao[0].P2 = filho->P1; // Antigo P1 vira o P2 da nova chave

    // 2. A chave separadora do pai desce para o início do filho
    filho->estacao[0].chave = pai->estacao[posFilho - 1].chave;
    filho->estacao[0].Pr = pai->estacao[posFilho - 1].Pr;
    filho->P1 = irmaoEsq.estacao[irmaoEsq.nroChaves - 1].P2; // Herda o último P2 do irmão esq
    filho->nroChaves++;

    // 3. A última chave do irmão esquerdo sobe para o pai
    pai->estacao[posFilho - 1].chave = irmaoEsq.estacao[irmaoEsq.nroChaves - 1].chave;
    pai->estacao[posFilho - 1].Pr = irmaoEsq.estacao[irmaoEsq.nroChaves - 1].Pr;

    // 4. Remove a última chave do irmão esquerdo
    arvoreb_removerChaveDoNo(&irmaoEsq, irmaoEsq.nroChaves - 1);

    // Salva as alterações
    arvoreb_escreverNoBin(arquivoIndiceBin, filho, rrnFilho);
    arvoreb_escreverNoBin(arquivoIndiceBin, &irmaoEsq, rrnEsq);
    arvoreb_escreverNoBin(arquivoIndiceBin, pai, rrnPai);
    return 1;
}

/**
 * @brief Tenta corrigir underflow concatenando o filho com seu irmão esquerdo.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
 * @param cabecalhoAB Ponteiro para o cabeçalho da árvore-B em RAM.
 * @param rrnPai RRN da página pai.
 * @param pai Ponteiro para o nó pai em RAM.
 * @param posFilho Posição lógica do filho em underflow dentro do pai.
 * @param rrnFilho RRN do filho em underflow.
 * @param filho Ponteiro para o nó filho em underflow.
 * @param rrnEsq RRN do irmão esquerdo.
 * * @return 1 se a redistribuição foi realizada, ou 0 caso contrário.
 */
int arvoreb_concatenarEsquerda(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int rrnPai, NO *pai, int posFilho, int rrnFilho, NO *filho, int rrnEsq) {
    if (rrnEsq == -1) return 0;
    if (posFilho <= 0) return 0; // Proteção de limite de índice no nó pai

    NO irmaoEsq = arvoreb_lerNoBin(arquivoIndiceBin, rrnEsq);

    // Proteção de capacidade: garante que a fusão não excederá o limite máximo de chaves
    if (irmaoEsq.nroChaves + 1 + filho->nroChaves > ORDEM_ARVORE - 1) {
        return 0;
    }

    // 1. Desce a chave do pai para o irmão esquerdo
    irmaoEsq.estacao[irmaoEsq.nroChaves].chave = pai->estacao[posFilho - 1].chave;
    irmaoEsq.estacao[irmaoEsq.nroChaves].Pr = pai->estacao[posFilho - 1].Pr;
    irmaoEsq.estacao[irmaoEsq.nroChaves].P2 = filho->P1;
    irmaoEsq.nroChaves++;

    // 2. Copia todas as chaves do filho (destruído) para o irmão esquerdo
    for (int i = 0; i < filho->nroChaves; i++) {
        irmaoEsq.estacao[irmaoEsq.nroChaves] = filho->estacao[i];
        irmaoEsq.nroChaves++;
    }

    // 3. Empilha a página que foi destruída (sempre a da direita no merge)
    arvoreb_empilharNoRemovido(arquivoIndiceBin, cabecalhoAB, rrnFilho);
    arvoreb_escreverNoBin(arquivoIndiceBin, &irmaoEsq, rrnEsq);

    // 4. Remove a chave do pai que desceu
    arvoreb_removerChaveDoNo(pai, posFilho - 1);
    arvoreb_escreverNoBin(arquivoIndiceBin, pai, rrnPai);
    
    return 1;
}

/**
 * @brief Corrige underflow concatenando o filho com seu irmão direito.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
 * @param cabecalhoAB Ponteiro para o cabeçalho da árvore-B em RAM.
 * @param rrnPai RRN da página pai.
 * @param pai Ponteiro para o nó pai em RAM.
 * @param posFilho Posição lógica do filho em underflow dentro do pai.
 * @param rrnFilho RRN do filho em underflow.
 * @param filho Ponteiro para o nó filho em underflow.
 * @param rrnDir RRN do irmão direito.
 * * @return 1 se a redistribuição foi realizada, ou 0 caso contrário.
 */
int arvoreb_concatenarDireita(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int rrnPai, NO *pai, int posFilho, int rrnFilho, NO *filho, int rrnDir) {
    if (rrnDir == -1) return 0;
    if (posFilho < 0 || posFilho >= pai->nroChaves) return 0; // Proteção de limite de índice no nó pai

    NO irmaoDir = arvoreb_lerNoBin(arquivoIndiceBin, rrnDir);

    // Proteção de capacidade: garante que a fusão não excederá o limite máximo de chaves
    if (filho->nroChaves + 1 + irmaoDir.nroChaves > ORDEM_ARVORE - 1) {
        return 0;
    }

    // 1. Desce a chave do pai para o filho
    filho->estacao[filho->nroChaves].chave = pai->estacao[posFilho].chave;
    filho->estacao[filho->nroChaves].Pr = pai->estacao[posFilho].Pr;
    filho->estacao[filho->nroChaves].P2 = irmaoDir.P1;
    filho->nroChaves++;

    // 2. Copia todas as chaves do irmão direito (destruído) para o filho
    for (int i = 0; i < irmaoDir.nroChaves; i++) {
        filho->estacao[filho->nroChaves] = irmaoDir.estacao[i];
        filho->nroChaves++;
    }

    // 3. Empilha a página que foi destruída (a da direita no merge)
    arvoreb_empilharNoRemovido(arquivoIndiceBin, cabecalhoAB, rrnDir);
    arvoreb_escreverNoBin(arquivoIndiceBin, filho, rrnFilho);

    // 4. Remove a chave do pai que desceu
    arvoreb_removerChaveDoNo(pai, posFilho);
    arvoreb_escreverNoBin(arquivoIndiceBin, pai, rrnPai);
    
    return 1;
}

/**
  * @brief Corrige o underflow de um filho da página pai de acordo com a sequência de redistribuição e concatenação.
  * *
  * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
  * @param cabecalhoAB Ponteiro para o cabeçalho da árvore-B em RAM.
  * @param rrnPai RRN da página pai.
  * @param pai Ponteiro para o nó pai em RAM.
  * @param posFilho Posição lógica do filho em underflow dentro do pai.
*/
void arvoreb_corrigirUnderflow(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int rrnPai, NO *pai, int posFilho) {
    int rrnFilho = arvoreb_getFilho(pai, posFilho);
    if (rrnFilho == -1) return; // Proteção contra leitura inválida

    NO filho = arvoreb_lerNoBin(arquivoIndiceBin, rrnFilho);

    // Identifica os irmãos adjacentes
    int rrnEsq = (posFilho > 0) ? arvoreb_getFilho(pai, posFilho - 1) : -1;
    int rrnDir = (posFilho < pai->nroChaves) ? arvoreb_getFilho(pai, posFilho + 1) : -1;

    // Seguindo a ordem especificada:

    // Prioridade 1: Redistribuição com a página adjacente à direita
    if (arvoreb_redistribuirDireita(arquivoIndiceBin, rrnPai, pai, posFilho, rrnFilho, &filho, rrnDir)) return;
    
    // Prioridade 2: Redistribuição com a página adjacente à esquerda
    if (arvoreb_redistribuirEsquerda(arquivoIndiceBin, rrnPai, pai, posFilho, rrnFilho, &filho, rrnEsq)) return;
    
    // Prioridade 3: Concatenação com a página adjacente à esquerda
    if (arvoreb_concatenarEsquerda(arquivoIndiceBin, cabecalhoAB, rrnPai, pai, posFilho, rrnFilho, &filho, rrnEsq)) return;
    
    // Prioridade 4: Concatenação com a página adjacente à direita
    if (arvoreb_concatenarDireita(arquivoIndiceBin, cabecalhoAB, rrnPai, pai, posFilho, rrnFilho, &filho, rrnDir)) return;
}

/* -------------------------------------------------------------------------- *
 * CHAMADAS PRINCIPAIS E AJUSTE DE RAIZ                                       *
 * -------------------------------------------------------------------------- */

/**
  * @brief Remove uma chave da árvore-B de forma recursiva.
  * *
  * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
  * @param cabecalhoAB Ponteiro para o cabeçalho da árvore-B em RAM.
  * @param rrnAtual RRN da página atual visitada na recursão.
  * @param chave Chave a ser removida da árvore-B.
  * *
  * @return 1 se a chave foi encontrada e removida, ou 0 caso contrário.
*/
int arvoreb_removerRecursivo(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int rrnAtual, int chave) {
    if (rrnAtual == -1) return 0; // Chave não existe na árvore
    
    NO atual = arvoreb_lerNoBin(arquivoIndiceBin, rrnAtual);
    int pos = arvoreb_encontrarPosicao(&atual, chave);
    
    // CASO 1: A chave está na página atual
    if (pos < atual.nroChaves && atual.estacao[pos].chave == chave) {
        if (atual.tipoNo == -1) { 
            // 1.A: É um nó folha. Remoção simples.
            arvoreb_removerChaveDoNo(&atual, pos);
            arvoreb_escreverNoBin(arquivoIndiceBin, &atual, rrnAtual);
        } else {
            // 1.B: É um nó intermediário ou raiz. Troca com a sucessora.
            int rrnFilhoDireita = arvoreb_getFilho(&atual, pos + 1);
            if (rrnFilhoDireita == -1) return 0; // Garantir integridade

            Estacao sucessora = arvoreb_buscarSucessora(arquivoIndiceBin, rrnFilhoDireita);
            if (sucessora.chave == -1) return 0; // Proteção p sucessora inválida
            
            // Substitui a chave que queremos apagar pela chave sucessora
            atual.estacao[pos].chave = sucessora.chave;
            atual.estacao[pos].Pr = sucessora.Pr;
            arvoreb_escreverNoBin(arquivoIndiceBin, &atual, rrnAtual);
            
            // Continua descendo para remover a cópia duplicada da chave sucessora
            arvoreb_removerRecursivo(arquivoIndiceBin, cabecalhoAB, rrnFilhoDireita, sucessora.chave);
            
            // Verifica a subárvore direita (se houve underflow)
            atual = arvoreb_lerNoBin(arquivoIndiceBin, rrnAtual);
            NO filhoDir = arvoreb_lerNoBin(arquivoIndiceBin, rrnFilhoDireita);
            
            // Se o filho existe, não foi removido no merge, e entrou em underflow
            if (filhoDir.removido == '0' && filhoDir.nroChaves < OCUPACAO_MINIMA) {
                arvoreb_corrigirUnderflow(arquivoIndiceBin, cabecalhoAB, rrnAtual, &atual, pos + 1);
            }
        }
    } 
    // CASO 2: Descer na árvore
    else {
        if (atual.tipoNo == -1) return 0; // Chegou na folha e a chave não existe
        
        int rrnFilho = arvoreb_getFilho(&atual, pos);
        int removeu = arvoreb_removerRecursivo(arquivoIndiceBin, cabecalhoAB, rrnFilho, chave);
        
        if (!removeu) return 0; // Encerra execução se nada foi removido
        
        // Verifica se o filho em que descemos está em underflow
        atual = arvoreb_lerNoBin(arquivoIndiceBin, rrnAtual);
        NO filho = arvoreb_lerNoBin(arquivoIndiceBin, rrnFilho);
        
        // Correção de Underflow
        if (filho.removido == '0' && filho.nroChaves < OCUPACAO_MINIMA) {
            arvoreb_corrigirUnderflow(arquivoIndiceBin, cabecalhoAB, rrnAtual, &atual, pos);
        }
    }
    
    return 1;
}

/**
  * @brief Ajusta a raiz da árvore-B após uma remoção.
  * *
  * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
  * @param cabecalhoAB Ponteiro para o cabeçalho da árvore-B em RAM.
*/
void arvoreb_ajustarRaiz(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB) {
    if (cabecalhoAB->noRaiz == -1) return;

    int rrnRaizAntiga = cabecalhoAB->noRaiz;
    NO raiz = arvoreb_lerNoBin(arquivoIndiceBin, rrnRaizAntiga);

    // Apenas ajusta se a raiz atual ficou vazia
    if (raiz.nroChaves != 0) return;

    if (raiz.tipoNo != -1) {
        int novaRaizRRN = raiz.P1;

        // Empilha a antiga raiz vazia
        arvoreb_empilharNoRemovido(arquivoIndiceBin, cabecalhoAB, rrnRaizAntiga);

        if (novaRaizRRN == -1) {
            cabecalhoAB->noRaiz = -1;
            return;
        }

        // Troca a raiz
        cabecalhoAB->noRaiz = novaRaizRRN;

        // Promove o filho e ajusta o tipoNo corretamente
        NO novaRaiz = arvoreb_lerNoBin(arquivoIndiceBin, novaRaizRRN);

        if (novaRaiz.P1 == -1) {
            novaRaiz.tipoNo = -1; // A nova raiz é uma folha
        } else {
            novaRaiz.tipoNo = 0;  // A nova raiz é intermediária
        }

        // Atualizar o índice
        arvoreb_escreverNoBin(arquivoIndiceBin, &novaRaiz, novaRaizRRN);
    } else {
        // A raiz era uma folha que ficou sem chaves
        arvoreb_empilharNoRemovido(arquivoIndiceBin, cabecalhoAB, rrnRaizAntiga);
        cabecalhoAB->noRaiz = -1;
    }
}

/**
  * @brief Função principal para remover uma chave da árvore-B (chama a recursão).
  * *
  * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
  * @param cabecalhoAB Ponteiro para o cabeçalho da árvore-B em RAM.
  * @param chave Chave a ser removida da árvore-B.
*/
void arvoreb_remover(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int chave) {
    if (cabecalhoAB->noRaiz == -1) return; // Árvore vazia
    
    int removeu = arvoreb_removerRecursivo(arquivoIndiceBin, cabecalhoAB, cabecalhoAB->noRaiz, chave);
    
    // Só precisamos verificar o status da raiz se houve uma remoção
    if (removeu) {
        arvoreb_ajustarRaiz(arquivoIndiceBin, cabecalhoAB);
    }
}