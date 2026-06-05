#include "arvoreb.h"

/* ========================================================================== *
 * FUNÇÕES DE GERENCIAMENTO DE CABEÇALHO                     *
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
    // A leitura de todos os campos ocorre sequencialmente, totalizando 17 bytes lidos.
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
    // Posiciona o cursor no byte 0 (início absoluto do arquivo) para sobrescrever o cabeçalho
    fseek(arquivoIndiceBin, 0, SEEK_SET);
    
    // Escrita sequencial dos 17 bytes correspondentes ao cabeçalho
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
    
    // Caso 1: Finalização segura do arquivo. Define status como '1' (consistente) e grava no disco.
    if (escreverConsistente == 1) {
        cabecalhoAB->status = '1'; 
        fseek(arquivoIndiceBin, 0, SEEK_SET);
        arvoreb_escreverCabecalhoBin(arquivoIndiceBin, cabecalhoAB);
        return 1;
    }
    
    // Caso 2: Verificação de segurança ao abrir o arquivo. Se estiver '0', aborta a operação.
    if (cabecalhoAB->status == '0') { 
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoIndiceBin);
        return 0;
    }

    // Caso 3: Abertura para escrita. Define status como '0' no disco imediatamente para prevenir corrupção em caso de falha.
    if (abertoParaLeitura == 0) {
        cabecalhoAB->status = '0';
        fseek(arquivoIndiceBin, 0, SEEK_SET);
        fwrite(&cabecalhoAB->status, sizeof(char), 1, arquivoIndiceBin); 
    }

    return 1;
}

/* ========================================================================== *
 * FUNÇÕES DE MANIPULAÇÃO DOS NÓS                        *
 * ========================================================================== */

/**
 * @brief Lê uma página (nó) específica da árvore-B diretamente do disco.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário da árvore-B.
 * @param RRN Registro Relativo Numérico indicando a posição do nó no disco.
 * @return NO Estrutura do nó preenchida com os dados lidos do arquivo.
 */
NO arvoreb_lerNoBin(FILE *arquivoIndiceBin, int RRN) {
    NO node; // Estrutura em RAM que irá armazenar os dados lidos

    // Posiciona o cursor de leitura saltando o cabeçalho (17 bytes) e as páginas anteriores (RRN * 53 bytes)
    fseek(arquivoIndiceBin, 17 + RRN * 53, SEEK_SET);

    // Leitura dos metadados da página
    fread(&node.removido, sizeof(char), 1, arquivoIndiceBin);   
    fread(&node.proximo, sizeof(int), 1, arquivoIndiceBin);
    fread(&node.tipoNo, sizeof(int), 1, arquivoIndiceBin);
    fread(&node.nroChaves, sizeof(int), 1, arquivoIndiceBin);

    // Leitura sequencial intercalada das Chaves de Busca (codEstacao) e Referências (byte offsets)
    for (int i = 0; i < ORDEM_ARVORE-1; i++) {
        fread(&node.estacao[i].chave, sizeof(int), 1, arquivoIndiceBin);
        fread(&node.estacao[i].Pr, sizeof(int), 1, arquivoIndiceBin);
    }
    
    // Leitura do ponteiro P1 (Filho esquerdo da primeira chave)
    fread(&node.P1, sizeof(int), 1, arquivoIndiceBin);

    // Leitura dos demais ponteiros (P2, P3 e P4) correspondentes às chaves lidas
    for (int i = 0; i < ORDEM_ARVORE-1; i++) 
        fread(&node.estacao[i].P2, sizeof(int), 1, arquivoIndiceBin);

    return node;
}

/**
 * @brief Instancia e inicializa um novo nó (página) da árvore-B na memória.
 * * @param cabecalhoAB Ponteiro para o registro de cabeçalho para atualização de métricas.
 * @param tipoNo Tipo do nó (-1: folha, 0: raiz, 1: intermediário).
 * @param P1 RRN do filho mais à esquerda (ou -1 se for folha).
 * @param estacao Vetor de estações que comporão inicialmente o nó.
 * @return NO Nova estrutura de nó pronta para ser inserida ou gravada.
 */
NO arvoreb_criarNoBin(CabecalhoAB *cabecalhoAB, int tipoNo, int P1, Estacao *estacao) {
    NO node;

    // Inicialização segura das flags de remoção lógica
    node.removido = '0';      
    node.proximo = -1;      
    node.tipoNo = tipoNo;

    // Definição da ocupação baseada no tipo do nó (raiz possui exceção matemática de mínimo 1 chave)
    if (tipoNo == 0 || (tipoNo == -1 && cabecalhoAB->nroNos == 0))
        node.nroChaves = 1;
    else 
        node.nroChaves = OCUPACAO_MINIMA;

    node.P1 = P1; // Âncora esquerda da página
    
    // Cópia profunda das chaves, referências e ponteiros direitos fornecidos
    for (int i = 0; i < node.nroChaves; i++) 
        node.estacao[i] = estacao[i];

    // Preenchimento de lixo (padding) lógico com -1 para os espaços não utilizados da página
    Estacao estacaoVazia = {-1, -1, -1};
    for (int i = node.nroChaves; i < ORDEM_ARVORE-1; i++) 
        node.estacao[i] = estacaoVazia;

    // Atualização global do cabeçalho refletindo a criação da nova página
    cabecalhoAB->nroNos++;
    cabecalhoAB->proxRRN++;

    return node;
}

/**
 * @brief Grava a estrutura de um nó (página) da RAM para a memória secundária (disco).
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário.
 * @param node Ponteiro para a estrutura contendo os dados a serem gravados.
 * @param RRN Registro Relativo Numérico alvo da gravação.
 */
void arvoreb_escreverNoBin(FILE *arquivoIndiceBin, NO *node, int RRN) {
    // Posiciona o cursor de gravação no offset exato da página-alvo (53 bytes por página)
    fseek(arquivoIndiceBin, 17 + RRN * 53, SEEK_SET);

    // Gravação dos metadados
    fwrite(&node->removido, sizeof(char), 1, arquivoIndiceBin);
    fwrite(&node->proximo, sizeof(int), 1, arquivoIndiceBin);
    fwrite(&node->tipoNo, sizeof(int), 1, arquivoIndiceBin);
    fwrite(&node->nroChaves, sizeof(int), 1, arquivoIndiceBin);

    // Gravação intercalada das Chaves de Busca (codEstacao) e Referências (byte offsets) de acordo com a especificação
    for (int i = 0; i < 3; i++) {
        fwrite(&node->estacao[i].chave, sizeof(int), 1, arquivoIndiceBin);
        fwrite(&node->estacao[i].Pr, sizeof(int), 1, arquivoIndiceBin);
    }

    // Gravação contígua dos ponteiros de subárvores no final da página
    fwrite(&node->P1, sizeof(int), 1, arquivoIndiceBin);
    for (int i = 0; i < 3; i++)
        fwrite(&node->estacao[i].P2, sizeof(int), 1, arquivoIndiceBin);
}

/* ========================================================================== *
 * FUNCIONALIDADES DE BUSCA                         *
 * ========================================================================== */

/**
 * @brief Realiza busca binária intra-nó para localizar uma chave ou determinar a subárvore correta.
 * * @param node Ponteiro para o nó onde a busca binária será executada.
 * @param chave Chave de busca (codEstacao) desejada.
 * @return Estacao* Retorna o endereço da estação exata (se encontrada) ou a estação que contém o ponteiro P2 do caminho de descida.
 */
Estacao* arvoreb_buscaBinaria(NO *node, int chave) {
    Estacao *retorno = NULL; // Ponteiro para armazenar o caminho de descida no caso da chave não estar neste nó

    int ini = 0, fim = node->nroChaves-1, mid;
    Estacao *estacao;
    
    // Algoritmo clássico de busca binária O(log n)
    while (ini <= fim) {
        mid = (ini+fim) >> 1; // Equivalente a (ini+fim)/2
        estacao = &node->estacao[mid];

        // Caso de sucesso: chave encontrada na página atual
        if (chave == estacao->chave)
            return estacao;

        // Se a chave buscada é maior, atualizamos o teto inferior e marcamos a estação como possível caminho de descida (via P2)
        if (chave > estacao->chave) {
            retorno = estacao;
            ini = mid+1;
        }
        // Se a chave buscada é menor, reduzimos o teto superior (descida será via P1 ou P2 de uma estação anterior)
        else {
            fim = mid-1;
        }
    }

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
    // Caso base: alcançou uma folha nula (chave não existe na árvore)
    if (nodeRRN == -1)
        return -1;

    // Carrega o nó atual da memória secundária
    NO node = arvoreb_lerNoBin(arquivoIndiceBin, nodeRRN);

    // Caso Especial P1: Chave menor que o limite inferior absoluto do nó. Desce pela subárvore mais à esquerda.
    if (chave < node.estacao[0].chave)
        return arvoreb_buscarRecursivo(arquivoIndiceBin, cabecalhoAB, node.P1, chave);

    // Executa a busca intra-nó para localizar a chave ou descobrir o ponteiro P2 apropriado
    Estacao *retorno = arvoreb_buscaBinaria(&node, chave);

    // Condição de parada de sucesso: a busca binária encontrou uma correspondência exata
    if (retorno->chave == chave) 
        return retorno->Pr;

    // Passo recursivo: a chave não está nesta página. Continua a busca na subárvore delimitada pelo ponteiro P2
    return arvoreb_buscarRecursivo(arquivoIndiceBin, cabecalhoAB, retorno->P2, chave);
}

/**
 * @brief Wrapper para inicializar a busca na árvore-B a partir do nó raiz.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário.
 * @param cabecalhoAB Ponteiro para o cabeçalho contendo o RRN da raiz.
 * @param chave Chave de busca.
 * @return int Retorna o offset do arquivo de dados ou -1 em caso de falha.
 */
int arvoreb_buscar(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int chave) {
    return arvoreb_buscarRecursivo(arquivoIndiceBin, cabecalhoAB, cabecalhoAB->noRaiz, chave);
}

/* ========================================================================== *
 * FUNCIONALIDADES DE INSERÇÃO                         *
 * ========================================================================== */

/**
 * @brief Insere ordenadamente uma nova estação em um vetor de estações pré-ordenado.
 * * @param estacoes Vetor contendo as chaves atuais (pode ser o próprio nó ou o vetor temporário de split).
 * @param estacaoParaInserir Ponteiro para a estação a ser acomodada.
 * @param nroChaves Quantidade atual de chaves já presentes no vetor.
 */
void arvoreb_ordenaNo(Estacao *estacoes, Estacao *estacaoParaInserir, int nroChaves) {
    // Deslocamento da direita para a esquerda (Insertion Sort local) para abrir espaço garantindo integridade de memória
    for (int i = nroChaves; i >= 0; i--) {
        // Encontra o ponto de inserção: extremo esquerdo (i==0) ou imediatamente após uma chave menor
        if (i == 0 || estacoes[i-1].chave < estacaoParaInserir->chave) {
            estacoes[i] = *estacaoParaInserir; // Cópia profunda mantendo a chave, Pr e Px consolidados
            break;
        }
        // Desloca o elemento atual uma posição à direita para acomodar a nova inserção
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

    // --- Etapa 1: Preparação do Vetor de Transbordo (Ordenação) ---
    
    Estacao estacoes[ORDEM_ARVORE]; // Vetor com capacidade para M chaves (causa do overflow)

    // Copia as chaves atuais da página original para o vetor temporário
    for (int i = 0; i < ORDEM_ARVORE-1; i++)
        estacoes[i] = node->estacao[i];

    // Insere a nova chave estourando o limite no vetor auxiliar, mantendo a ordem estrita
    arvoreb_ordenaNo(estacoes, estacaoParaInserir, ORDEM_ARVORE-1);

    // --- Etapa 2: Definição da Mediana e Distribuição de Chaves ---

    // Identificação da chave mediana que será promovida para o nó pai
    Estacao estacaoPromovida = estacoes[ORDEM_ARVORE/2];

    NO novoNo; // Estrutura que irá comportar a metade direita da página original dividida
    
    // Preenchimento do nó direito com as chaves maiores (à direita da mediana)
    for (int i = 0; i < OCUPACAO_MINIMA; i++)
        novoNo.estacao[i] = estacoes[ORDEM_ARVORE/2 + 1 + i];

    // Transferência essencial de ponteiros: o filho mais à esquerda do novo nó herda o ponteiro direito da chave promovida
    novoNo.P1 = estacaoPromovida.P2;
    // O ponteiro direito da chave promovida passa a apontar para o RRN da nova página que será alocada
    estacaoPromovida.P2 = cabecalhoAB->proxRRN;

    // Atualização da página original (metade esquerda) mantendo as chaves menores
    for (int i = 0; i < ORDEM_ARVORE/2; i++)
        node->estacao[i] = estacoes[i];

    // Limpeza por preenchimento (padding nulo) nas posições excedentes da página original
    Estacao estacaoVazia = {-1, -1, -1};
    for (int i = ORDEM_ARVORE/2; i < ORDEM_ARVORE-1; i++)
        node->estacao[i] = estacaoVazia;

    // Redução do número de chaves refletindo a divisão
    node->nroChaves = ORDEM_ARVORE/2;

    // --- Etapa 3: Tratamento e Hierarquia de Tipos de Nó ---
    
    int tipoNoAntigo = node->tipoNo;
    int nroNos = cabecalhoAB->nroNos;
   
    // A página direita espelha as características de nível de folha do nó pai antes de dividi-lo
    novoNo.tipoNo = -2; // Valor âncora provisório
    if (node->tipoNo == -1)
        novoNo.tipoNo = -1;
    else {
        // Se não for folha, verifica a existência de filhos remanescentes para confirmar a classificação
        int filhos = 0;
        if (node->P1 != -1) filhos = 1;
        for (int i = 0; i < node->nroChaves && filhos == 0; i++)
            if (node->estacao[i].P2 != -1) filhos = 1;
         
        // Se houver filhos, garante a configuração como intermediário, caso contrário, folha.
        if (filhos) node->tipoNo = 1;
        else node->tipoNo = -1;
    }

    // Persiste o nó original atualizado no disco
    arvoreb_escreverNoBin(arquivoIndiceBin, node, nodeRRN);

    // Repete a verificação e classificação de hierarquia estrutural para o novo nó direito
    if (novoNo.tipoNo != -1) {
        int filhos = 0;
        if (novoNo.P1 != -1) filhos = 1;
        for (int i = 0; i < OCUPACAO_MINIMA && filhos == 0; i++)
            if (novoNo.estacao[i].P2 != -1) filhos = 1;
         
        if (filhos) novoNo.tipoNo = 1;
        else novoNo.tipoNo = -1;
    }

    // --- Etapa 4: Criação e Gravação Definitiva ---

    int RRNNovoNO = cabecalhoAB->proxRRN;
    // O novo nó é formatado segundo as regras da página com o reaproveitamento da estrutura alocada em RAM
    novoNo = arvoreb_criarNoBin(cabecalhoAB, novoNo.tipoNo, novoNo.P1, novoNo.estacao);
    arvoreb_escreverNoBin(arquivoIndiceBin, &novoNo, RRNNovoNO);
    
    // Tratamento excepcional da Raiz: Se o split ocorreu no nó raiz original (mesmo que estivesse operando como folha), force o crescimento da árvore criando um novo nível
    if (tipoNoAntigo == 0 || (tipoNoAntigo == -1 && nroNos == 1)) {
        cabecalhoAB->noRaiz = cabecalhoAB->proxRRN; // Atualiza a diretriz da raiz no registro de controle
        int RRNNovaRaiz = cabecalhoAB->proxRRN;
        
        // A nova raiz recém-criada é indexada abraçando as duas metades provenientes do split
        NO novaRaiz = arvoreb_criarNoBin(cabecalhoAB, 0, nodeRRN, &estacaoPromovida);
        arvoreb_escreverNoBin(arquivoIndiceBin, &novaRaiz, RRNNovaRaiz);
    } 

    return estacaoPromovida; // Devolve a chave mediana para a cadeia recursiva tratar (ou descartar se foi resolvida na raiz)
}

/**
 * @brief Fluxo recursivo principal de inserção de chave ou tratativa de promoção em cascata.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário.
 * @param cabecalhoAB Ponteiro para o cabeçalho.
 * @param nodeRRN RRN da página alvo desta etapa da recursão.
 * @param estacaoParaInserir Estação contendo a chave candidata a inserção.
 * @return Estacao Retorna uma estação formatada se houver cascata de promoção (split interno) ou chave nula se a operação finalizou no nível inferior.
 */
Estacao arvoreb_inserirRecursivo(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, int nodeRRN, Estacao estacaoParaInserir) {
    NO node = arvoreb_lerNoBin(arquivoIndiceBin, nodeRRN);

    // Passo de Descida: Se a página não for folha (exceção se for apenas 1 nó solitário), direcione o fluxo em profundidade
    if (node.tipoNo != -1 && cabecalhoAB->nroNos > 1) {
            
        // Caso de desvio pelo filho mais à esquerda (âncora)
        if (estacaoParaInserir.chave < node.estacao[0].chave)
            estacaoParaInserir = arvoreb_inserirRecursivo(arquivoIndiceBin, cabecalhoAB, node.P1, estacaoParaInserir);
        else {
            Estacao *retorno = arvoreb_buscaBinaria(&node, estacaoParaInserir.chave);

            // Regra de Integridade: Se a chave já existir no banco, a inserção é abortada devolvendo um código de cancelamento negativo
            if (retorno->chave == estacaoParaInserir.chave) {
                estacaoParaInserir.chave = -1;
                return estacaoParaInserir;
            }
            // Chama a recursão pela subárvore direita delimitada na busca
            estacaoParaInserir = arvoreb_inserirRecursivo(arquivoIndiceBin, cabecalhoAB, retorno->P2, estacaoParaInserir);
        }
    }

    // Resolução: Condição na qual a propagação foi concluída silenciosamente ou o nó já existia.
    if (estacaoParaInserir.chave < 0)
        return estacaoParaInserir;

    // Inserção Efetiva em Nível Atual (Folha ou Propagação de Interno)
    
    // Possibilidade A: A página possui espaço disponível para acomodação
    if (node.nroChaves < 3) {
        arvoreb_ordenaNo(node.estacao, &estacaoParaInserir, node.nroChaves); // Acopla garantindo integridade e ponteiros
        node.nroChaves++;

        estacaoParaInserir.chave = -1; // Conclui o processo mascarando o retorno (não exige mais propagação ao pai)
        arvoreb_escreverNoBin(arquivoIndiceBin, &node, nodeRRN); // Commit dos dados da página no disco
    }
    // Possibilidade B: Página sobrecarregada deflagra processo de cisão
    else {
        estacaoParaInserir = arvoreb_split(arquivoIndiceBin, cabecalhoAB, &estacaoParaInserir, &node, nodeRRN);
    }

    return estacaoParaInserir; // Retorna status ao nível superior
}

/**
 * @brief Função principal encapsuladora para solicitar uma inserção na estrutura da árvore-B.
 * * @param arquivoIndiceBin Ponteiro para o arquivo binário do índice.
 * @param cabecalhoAB Ponteiro para o cabeçalho em RAM.
 * @param estacaoParaInserir Estação (chave + byte offset) inicial lida do arquivo de dados que desencadeia a operação.
 */
void arvoreb_inserir(FILE *arquivoIndiceBin, CabecalhoAB *cabecalhoAB, Estacao estacaoParaInserir) {
    NO node;

    // Condição Gênesis: Árvore completamente limpa sem raiz inicializada
    if (cabecalhoAB->noRaiz == -1) {
        cabecalhoAB->noRaiz = cabecalhoAB->proxRRN;
        
        // Cria primeira e única página da árvore formatada atuando simultaneamente como folha e raiz
        node = arvoreb_criarNoBin(cabecalhoAB, -1, -1, &estacaoParaInserir);
        arvoreb_escreverNoBin(arquivoIndiceBin, &node, cabecalhoAB->noRaiz);
        return;
    }
    
    // Inicia cascata top-down delegando o processo ao fluxo recursivo profundo
    arvoreb_inserirRecursivo(arquivoIndiceBin, cabecalhoAB, cabecalhoAB->noRaiz, estacaoParaInserir);
}