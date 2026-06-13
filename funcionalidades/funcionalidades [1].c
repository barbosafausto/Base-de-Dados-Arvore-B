#include "funcionalidades.h"
#include <stdio.h>

/* ========================================================================== *
 * FUNCIONALIDADE [7] - CREATE INDEX                                          *
 * ========================================================================== */

/**
 * @brief Funcionalidade [7]: Cria um arquivo de índice Árvore-B a partir de um arquivo de dados.
 * O campo indexado é sempre a chave primária 'codEstacao'. Registros logicamente
 * removidos no arquivo de dados são ignorados e não compõem a Árvore-B.
 * * @param nomeArquivoDadosBin Nome do arquivo binário de dados (entrada).
 * @param nomeArquivoIndiceBin Nome do arquivo binário de índice (saída/árvore-B).
 */
void createIndex(char* nomeArquivoDadosBin, char* nomeArquivoIndiceBin) {
    
    // --- Abrir arquivos ---
    
    // Abertura do arquivo de dados para leitura binária
    FILE *arquivoDadosBin = fopen(nomeArquivoDadosBin, "rb");
    if(arquivoDadosBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }
    
    // --- Verificação de consistência do arquivo de dados ---
    Cabecalho cabecalho; 
    fread(&cabecalho.status, sizeof(char), 1, arquivoDadosBin);
    if (!registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 0, 1)) {
        return;
    }

    // Abertura/criação do arquivo de índice para escrita e leitura binária
    FILE *arquivoIndiceBin = fopen(nomeArquivoIndiceBin, "wb+");
    if(arquivoIndiceBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoDadosBin);
        return;
    }

    // --- Inicialização e Escrita do Cabeçalho da Árvore-B ---
    
    // A função initCabecalho define o status inicial como '0' (inconsistente durante a criação)
    CabecalhoAB cabecalhoAB;
    arvoreb_initCabecalho(&cabecalhoAB);
    
    // O cabeçalho provisório é gravado no início do arquivo de índice
    arvoreb_escreverCabecalhoBin(arquivoIndiceBin, &cabecalhoAB);

    // --- Leitura Sequencial e Indexação ---
    
    // Posiciona o cursor do arquivo de dados logo após o seu cabeçalho (byte 17)
    fseek(arquivoDadosBin, 17, SEEK_SET);

    // Variáveis auxiliares para a leitura dos registros
    char removido;
    int codEstacao;
    int offsetAtual = 17; // Mantém o rastreio físico do registro atual (byte offset)
    int RRN;              // Variável declarada, pronta para uso futuro se necessário

    // Loop de extração: lê o byte 'removido' de cada registro até o fim do arquivo
    while (fread(&removido, sizeof(char), 1, arquivoDadosBin) == 1) {

        // Tratamento de registros logicamente removidos
        if (removido == '1') {
            // Se removido, incrementa o offset com o tamanho total do registro (80 bytes)
            // e salta fisicamente para o próximo registro, ignorando os dados atuais
            offsetAtual += 80;
            fseek(arquivoDadosBin, offsetAtual, SEEK_SET);        
            continue;
        }

        // --- Processamento de Registro Válido ---

        // Salta o campo 'proximo' (4 bytes) da pilha de removidos. 
        // Como o cursor estava após o 'removido' (offsetAtual + 1), saltamos para offsetAtual + 5
        fseek(arquivoDadosBin, offsetAtual + 5, SEEK_SET);

        // Extrai a chave primária que será indexada
        fread(&codEstacao, sizeof(int), 1, arquivoDadosBin);

        // Empacota a chave e o seu byte offset na estrutura Estacao (Pr = offsetAtual)
        Estacao estacaoParaInserir = {codEstacao, offsetAtual, -1};

        // Aciona a rotina de inserção top-down na Árvore-B
        arvoreb_inserir(arquivoIndiceBin, &cabecalhoAB, estacaoParaInserir);

        // Atualiza os ponteiros de rastreio para o próximo ciclo de leitura
        offsetAtual += 80;
        fseek(arquivoDadosBin, offsetAtual, SEEK_SET);
    }

    // --- Finalização ---
    
    // Após indexar todos os dados, atualiza a consistência da árvore para '1' (estável)
    fseek(arquivoIndiceBin, 0, SEEK_SET);
    arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 1, 0);
    
    // Fecha os descritores de arquivo para liberar a memória do S.O.
    fclose(arquivoDadosBin);
    fclose(arquivoIndiceBin);

    // Exibe o hash hexadecimal do arquivo gerado para a correção do RunCodes
    BinarioNaTela(nomeArquivoIndiceBin);
}


/* ========================================================================== *
 * FUNCIONALIDADE [8] - SELECT FROM WHERE COM ÁRVORE-B                        *
 * ========================================================================== */

/**
 * @brief Funcionalidade [8]: Realiza buscas com critérios de seleção otimizadas pela Árvore-B.
 * Se a busca envolver a chave primária 'codEstacao', utiliza o índice para busca em O(log n).
 * Caso contrário, delega para a busca sequencial clássica.
 * * @param nomeArquivoDadosBin Nome do arquivo de dados.
 * @param nomeArquivoIndiceBin Nome do arquivo de índice Árvore-B.
 * @param nBuscas Quantidade de pesquisas que serão efetuadas em sequência.
 */
void selectWhereAB(char *nomeArquivoDadosBin, char *nomeArquivoIndiceBin, int nBuscas) {

    // --- Abertura e Validação do Arquivo de Dados ---
    FILE *arquivoDadosBin = fopen(nomeArquivoDadosBin, "rb");
    if(arquivoDadosBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    Cabecalho cabecalho; 
    fread(&cabecalho.status, sizeof(char), 1, arquivoDadosBin);
    if (!registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 0, 1)) {
        return;
    }

    int temArvoreB = 1;
    if (nomeArquivoIndiceBin == NULL)
        temArvoreB = 0;

    // Só lidamos com o arquivo de índice se a árvore B existir.
    FILE *arquivoIndiceBin = NULL;
    CabecalhoAB cabecalhoAB;
    if (temArvoreB) {
        // --- Abertura e Validação do Arquivo de Índice ---
        arquivoIndiceBin = fopen(nomeArquivoIndiceBin, "rb");
        if(arquivoIndiceBin == NULL) {
            printf("Falha no processamento do arquivo.\n");
            fclose(arquivoDadosBin);
            return;
        }
        
        arvoreb_lerCabecalhoBin(arquivoIndiceBin, &cabecalhoAB);
        if (!arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 0, 1)) {
            return;
        }
    }

    // --- Processamento de Buscas ---
    
    // Itera pela quantidade de buscas especificadas na entrada
    for (int i = 0; i < nBuscas; i++) {

        // Instancia a estrutura genérica de busca e recebe os filtros de entrada
        Busca busca;
        int codEstacao = utils_recebeCampos(&busca);

        // Variável que armazenará a posição do registro no disco
        int byteOffset = -1;
        
        // --- Decisão de Algoritmo ---
        
        // Se a chave primária não foi informada nos filtros de busca OU se não tem árvore-B
        // Vamos fazer o select tradicional
        if (codEstacao == -1 || !temArvoreB) {
            // Delega a pesquisa para a funcionalidade [3] (Busca Sequencial)
            selectWhere(arquivoDadosBin, &busca);
            continue; 
        }   
        else {
            // Se a chave foi informada, utiliza o índice para acesso direto
            byteOffset = arvoreb_buscar(arquivoIndiceBin, &cabecalhoAB, codEstacao); 
        }

        // --- Recuperação e Exibição do Registro ---
        
        Registro registro;
        // Se o offset retornou nulo (-1), a chave não existe na Árvore-B
        if (byteOffset == -1) {
            printf("Registro inexistente.\n\n");
            continue;
        }
        
        // Caso a chave exista, salta com precisão O(1) diretamente para o byte de início do registro
        fseek(arquivoDadosBin, byteOffset, SEEK_SET);
        registro_lerRegistroBin(arquivoDadosBin, &registro);

        // Validação Secundária: Verifica se o registro recuperado atende a TODOS os outros
        // filtros que possam ter sido combinados com o 'codEstacao' na mesma busca
        if (utils_compararRegistroComFiltros(&registro, &busca)) {
            registro_imprimirRegistro(&registro);
            printf("\n");
        }
        // Se falhou nos critérios adicionais, o conjunto resultante é vazio
        else {
            printf("Registro inexistente.\n\n");
        }
    }
    
    // Fechamento dos arquivos 
    fclose(arquivoDadosBin);
    if (temArvoreB) fclose(arquivoIndiceBin);
}

/* ========================================================================== *
 * FUNCIONALIDADE [9] - INSERT INTO NA ÁRVORE-B E NO ARQUIVO DE ÍNDICE        *
 * ========================================================================== */

/**
 * @brief Funcionalidade [9]: Insere registros no arquivo de dados.
 * @param nomeArquivoDadosBin Nome do arquivo de dados.
 * @param nomeArquivoIndiceBin Nome do arquivo de índice Árvore-B.
 * @param nInsercoes Quantidade de registros a serem inseridos.
 */
void insertIntoAB(char *nomeArquivoDadosBin, char *nomeArquivoIndiceBin, int nInsercoes){

    // --- Abertura e Validação do Arquivo de Dados / Arquivo de Índice ---
    FILE *arquivoDadosBin = fopen(nomeArquivoDadosBin, "rb+");
    if(arquivoDadosBin == NULL){
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    FILE *arquivoIndiceBin = fopen(nomeArquivoIndiceBin, "rb+");
    if(arquivoIndiceBin == NULL){
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoDadosBin);
        return;
    }


    // --- Verificando consistência dos arquivos ---
    Cabecalho cabecalho;
    registro_lerCabecalho(arquivoDadosBin, &cabecalho);

    if(!registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 0, 1)){
        fclose(arquivoIndiceBin);
        return;
    }

    CabecalhoAB cabecalhoAB;
    arvoreb_lerCabecalhoBin(arquivoIndiceBin, &cabecalhoAB);

    if(!arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 0, 1)){
        fclose(arquivoDadosBin);
        return;
    }

    registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 0, 0);
    arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 0, 0);


    // Loop de inserção
    for(int i = 0; i < nInsercoes; i++){

        Registro registro;
        registro_lerRegistro(&registro);

        // Se a chave já existe na árvore-B, não insere no arquivo de dados nem no índice.
        if (arvoreb_buscar(arquivoIndiceBin, &cabecalhoAB, registro.codEstacao) != -1) {
            continue;
        }

        int byteOffsetInserir;

        // Caso 1: existe registro removido para reaproveitar
        if(cabecalho.topo != -1){

            int rrnReusado = cabecalho.topo;
            byteOffsetInserir = TAM_CABECALHO + rrnReusado * TAM_REGISTRO;

            // Consumir o próximo RRN da pilha para não perder a referência
            fseek(arquivoDadosBin, byteOffsetInserir + 1, SEEK_SET);
            fread(&cabecalho.topo, sizeof(int), 1, arquivoDadosBin);

            // Sobrescrever o registro removido pelo novo
            fseek(arquivoDadosBin, byteOffsetInserir, SEEK_SET);
            registro_escreverRegistroBin(arquivoDadosBin, &registro);
        }

        // Caso 2: topo == -1 (sem registro removido)
        else {
            
            byteOffsetInserir = TAM_CABECALHO + cabecalho.proxRRN * TAM_REGISTRO;

            fseek(arquivoDadosBin, byteOffsetInserir, SEEK_SET);
            registro_escreverRegistroBin(arquivoDadosBin, &registro);

            cabecalho.proxRRN++;
        }

        // Atualizar árvore-B (chave codEstação e offset do novo registro)
        Estacao estacaoInserir = {
            registro.codEstacao,
            byteOffsetInserir,
            -1
        };

        // Inserir na Árvore-B
        arvoreb_inserir(arquivoIndiceBin, &cabecalhoAB, estacaoInserir);

    }

    // Recalcula informações do cabeçalho do arquivo de dados
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoDadosBin, 1);

    // Finaliza os arquivos como consistentes
    registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 1, 0);
    arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 1, 0);

    fclose(arquivoDadosBin);
    fclose(arquivoIndiceBin);

    BinarioNaTela(nomeArquivoDadosBin);
    BinarioNaTela(nomeArquivoIndiceBin);

}

/* ========================================================================== *
 * FUNCIONALIDADE [10] - DELETE WHERE NA ÁRVORE-B E NO ARQUIVO DE ÍNDICE      *
 * ========================================================================== */

/**
 * @brief Funcionalidade [10]: Deleta registros a partir de um filtro (WHERE). Remoção lógica
 * @param nomeArquivoDadosBin Nome do arquivo de dados.
 * @param nomeArquivoIndiceBin Nome do arquivo de índice Árvore-B.
 * @param nRemocoes Quantidade de registros a serem removidos.
 */
void deleteWhereAB(char *nomeArquivoDadosBin, char *nomeArquivoIndiceBin, int nRemocoes){

    // Abrindo os arquivos
    FILE *arquivoDadosBin = fopen(nomeArquivoDadosBin, "rb+");
    if(arquivoDadosBin == NULL){
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    FILE *arquivoIndiceBin = fopen(nomeArquivoIndiceBin, "rb+");
    if(arquivoIndiceBin == NULL){
        printf("Falha no processamento do arquivo\n");
        return;
    }

    // Validando cabeçalhos
    Cabecalho cabecalho;
    registro_lerCabecalho(arquivoDadosBin, &cabecalho);

    if(!registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 0, 1)){
        fclose(arquivoIndiceBin);
        return;
    }

    CabecalhoAB cabecalhoAB;
    arvoreb_lerCabecalhoBin(arquivoIndiceBin, &cabecalhoAB);

    if(!arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 0, 1)){
        fclose(arquivoDadosBin);
        return;
    }

    registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 0, 0);
    arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 0, 0);


    // Loop remoção
    for(int i = 0; i < nRemocoes; i++){

        Busca busca;
        int codEstacao = utils_recebeCampos(&busca);

        // Caso 1: Remoção com codEstação (busca na árvore-B)
        if(codEstacao != -1) {

                int byteOffset = arvoreb_buscar(arquivoIndiceBin, &cabecalhoAB, codEstacao);

                // Caso ache o Offset
                if(byteOffset != -1) {

                    Registro registro;
                    fseek(arquivoDadosBin, byteOffset, SEEK_SET);
                    registro_lerRegistroBin(arquivoDadosBin, &registro);

                    if(registro.removido != '1' && utils_compararRegistroComFiltros(&registro, &busca)){
                        int chaveRemocao = registro.codEstacao;

                        registro_deletarRegistro(&registro, &cabecalho, arquivoDadosBin, byteOffset + TAM_REGISTRO);

                        arvoreb_remover(arquivoIndiceBin, &cabecalhoAB, chaveRemocao);
                    }
                }
        }

        // Caso 2: Remoção com outros filtros (sequencial)
        else {

            // Pular cabeçalho
            fseek(arquivoDadosBin, TAM_CABECALHO, SEEK_SET);

            Registro registro;
            int offsetAtual = TAM_CABECALHO;

            // Percorrer de forma sequencial
            while(registro_lerRegistroBin(arquivoDadosBin, &registro) != -1){
                
                offsetAtual += TAM_REGISTRO;

                // Registro já removido
                if(registro.removido == '1') continue;

                // Se o registro for compativel com a busca
                if(utils_compararRegistroComFiltros(&registro, &busca)){
                    
                    int chaveRemocao = registro.codEstacao;

                    registro_deletarRegistro(&registro, &cabecalho, arquivoDadosBin, offsetAtual);

                    arvoreb_remover(arquivoIndiceBin, &cabecalhoAB, chaveRemocao);

                }
            }
        }

        free(busca.campo);
    }

    // Atualizar valores dos Pares
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoDadosBin, 1);

    // Deixar consistente
    registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 1, 0);
    arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 1, 0);

    fclose(arquivoDadosBin);
    fclose(arquivoIndiceBin);

    BinarioNaTela(nomeArquivoDadosBin);
    BinarioNaTela(nomeArquivoIndiceBin);

}