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