#include "funcionalidades.h"
#include <stdio.h>

/* ========================================================================== *
 * FUNCIONALIDADE [7] - CREATE INDEX (ÁRVORE-B)                               *
 * ========================================================================== */

/**
 * @brief Funcionalidade [7]: Cria um arquivo de índice Árvore-B a partir de um arquivo de dados.
 * O campo indexado é sempre a chave primária 'codEstacao'. Registros logicamente
 * removidos no arquivo de dados são ignorados e não compõem a Árvore-B.
 * * @param nomeArquivoDadosBin Nome do arquivo binário de dados (entrada).
 * @param nomeArquivoIndiceBin Nome do arquivo binário de índice (saída/árvore-B).
 */
void createIndex(char* nomeArquivoDadosBin, char* nomeArquivoIndiceBin) {
    
    // --- Abertura e Validação do Arquivo de Dados ---
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

    // --- Abertura/Criação do Arquivo de Índice ---
    FILE *arquivoIndiceBin = fopen(nomeArquivoIndiceBin, "wb+");
    if(arquivoIndiceBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoDadosBin);
        return;
    }

    // --- Inicialização e Escrita do Cabeçalho da Árvore-B ---
    CabecalhoAB cabecalhoAB;
    arvoreb_initCabecalho(&cabecalhoAB);
    
    // O cabeçalho provisório é gravado no início do arquivo de índice (status '0': inconsistente)
    arvoreb_escreverCabecalhoBin(arquivoIndiceBin, &cabecalhoAB);

    // --- Leitura Sequencial e Indexação ---
    // Posiciona o cursor do arquivo de dados logo após o seu cabeçalho (byte 17)
    fseek(arquivoDadosBin, TAM_CABECALHO, SEEK_SET);

    char removido;
    int codEstacao;
    int offsetAtual = 17; // Rastreamento do offset do registro

    // Loop pelo arquivo de dados: lê o byte 'removido' de cada registro até o EOF
    while (fread(&removido, sizeof(char), 1, arquivoDadosBin) == 1) {

        // Tratamento de registros logicamente removidos
        if (removido == '1') {

            offsetAtual += 80;
            fseek(arquivoDadosBin, offsetAtual, SEEK_SET);        
            continue;
        }

        // --- Processamento de Registro Válido ---
        // Salta o campo 'proximo' (4 bytes). Cursor vai para offsetAtual + 5
        fseek(arquivoDadosBin, offsetAtual + 5, SEEK_SET);

        // Extrai a chave primária que será indexada
        fread(&codEstacao, sizeof(int), 1, arquivoDadosBin);

        // "Empacota" a chave e o seu byte offset na estrutura Estacao (Pr = offsetAtual)
        Estacao estacaoParaInserir = {codEstacao, offsetAtual, -1};

        // Chama a rotina de inserção top-down na Árvore-B
        arvoreb_inserir(arquivoIndiceBin, &cabecalhoAB, estacaoParaInserir);

        // Atualiza o offsetAtual para o próximo ciclo
        offsetAtual += 80;
        fseek(arquivoDadosBin, offsetAtual, SEEK_SET);
    }

    // --- Finalização ---
    // Atualiza a consistência da árvore para '1' (estável) e grava o cabeçalho
    fseek(arquivoIndiceBin, 0, SEEK_SET);
    arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 1, 0);
    
    fclose(arquivoDadosBin);
    fclose(arquivoIndiceBin);

    BinarioNaTela(nomeArquivoIndiceBin);
}


/* ========================================================================== *
 * FUNCIONALIDADE [8] - SELECT FROM WHERE COM ÁRVORE-B                        *
 * ========================================================================== */

/**
 * @brief Funcionalidade [8]: Realiza buscas com critérios de seleção otimizadas pela Árvore-B.
 * Se a busca envolver a chave primária 'codEstacao', utiliza o índice para busca.
 * Caso contrário, delega para a busca sequencial clássica (funcionalidade 3).
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

    // Verificando se foi fornecido um nome para arquivo de árvore-B
    // Se não foi fornecido, chamaremos a funcionalidade 3.
    int temArvoreB = 1;
    if (nomeArquivoIndiceBin == NULL)
        temArvoreB = 0;

    // --- Abertura e Validação do Arquivo de Índice (se aplicável) ---
    FILE *arquivoIndiceBin = NULL;
    CabecalhoAB cabecalhoAB;
    
    if (temArvoreB) {

        // --- Se tem árvore-b, então abrimos o arquivo e verificamos sua integridade ---
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
    for (int i = 0; i < nBuscas; i++) {

        // Recebimento dos campos
        Busca busca;
        int codEstacao = utils_recebeCampos(&busca);
        int byteOffset = -1;
        
        // --- Decisão de Algoritmo ---
        // Delega para busca sequencial clássica caso a chave não seja informada
        // ou se a árvore não foi disponibilizada como argumento.
        if (codEstacao == -1 || !temArvoreB) {
            selectWhere(arquivoDadosBin, &busca);
            continue; 
        }   
        else {
            // Busca na árvore 
            byteOffset = arvoreb_buscar(arquivoIndiceBin, &cabecalhoAB, codEstacao); 
        }

        // --- Recuperação e Exibição do Registro ---
        Registro registro;
        
        // Chave inexistente na Árvore-B
        if (byteOffset == -1) {
            printf("Registro inexistente.\n\n");
            free(busca.campo); // Lembrando de liberar os campos atuais
            continue;
        }
        
        // Se chegou aqui, então achamamos uma chave válida. Vamos ler o registro
        fseek(arquivoDadosBin, byteOffset, SEEK_SET);
        registro_lerRegistroBin(arquivoDadosBin, &registro);

        // Validação Secundária: verifica se os demais filtros fornecidos também dão 'match'
        if (utils_compararRegistroComFiltros(&registro, &busca)) {
            registro_imprimirRegistro(&registro);
            printf("\n");
        }
        else {
            printf("Registro inexistente.\n\n");
        }

        // Liberação da alocação dinâmica
        free(busca.campo);
    }
    
    // Fechamento seguro
    fclose(arquivoDadosBin);
    if (temArvoreB) fclose(arquivoIndiceBin);
}

/* ========================================================================== *
 * FUNCIONALIDADE [9] - INSERT INTO COM ÁRVORE-B                              *
 * ========================================================================== */

/**
 * @brief Funcionalidade [9]: Insere novos registros no arquivo de dados mantendo o índice atualizado.
 * Reutiliza espaços logicamente removidos (pilha) ou anexa no final. A inserção só prossegue 
 * se a chave primária não existir na Árvore-B.
 * * @param nomeArquivoDadosBin Nome do arquivo de dados.
 * @param nomeArquivoIndiceBin Nome do arquivo de índice Árvore-B.
 * @param nInsercoes Quantidade de registros a serem inseridos.
 */
void insertIntoAB(char *nomeArquivoDadosBin, char *nomeArquivoIndiceBin, int nInsercoes){

    // --- Abertura de Arquivos em Modo Misto ---
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

    // --- Verificando e Marcando Consistência ---
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

    // Marca ambos os arquivos como inconsistentes (status '0') pois sofrerão mutação
    registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 0, 0);
    arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 0, 0);


    // --- Loop de inserção ---
    for(int i = 0; i < nInsercoes; i++){

        Registro registro;
        registro_lerRegistro(&registro);

        // Regra de Unicidade: ignora a inserção se a chave já existe no Índice Árvore-B
        if (arvoreb_buscar(arquivoIndiceBin, &cabecalhoAB, registro.codEstacao) != -1) {
            continue;
        }

        int byteOffsetInserir;

        // Caso 1: Reutilização de espaço fragmentado (LIFO)
        if(cabecalho.topo != -1){

            // Apontar corretamente para o endereço a ser reaproveitado
            int rrnReusado = cabecalho.topo;
            byteOffsetInserir = TAM_CABECALHO + rrnReusado * TAM_REGISTRO;

            // Avança para ler o próximo item da pilha antes de sobrescrever
            fseek(arquivoDadosBin, byteOffsetInserir + 1, SEEK_SET);
            fread(&cabecalho.topo, sizeof(int), 1, arquivoDadosBin);

            // Retorna ao offset basal e escreve o novo registro
            fseek(arquivoDadosBin, byteOffsetInserir, SEEK_SET);
            registro_escreverRegistroBin(arquivoDadosBin, &registro);
        }
        // Caso 2: Apendagem sequencial no fim do arquivo
        else {
            // Apontar para o próximo RRN, já que não tem o que reaproveitar
            byteOffsetInserir = TAM_CABECALHO + cabecalho.proxRRN * TAM_REGISTRO;

            fseek(arquivoDadosBin, byteOffsetInserir, SEEK_SET);
            registro_escreverRegistroBin(arquivoDadosBin, &registro);

            cabecalho.proxRRN++;
        }

        // --- Atualização Imediata do Índice Árvore-B ---
        Estacao estacaoInserir = {
            registro.codEstacao,
            byteOffsetInserir,
            -1
        };

        arvoreb_inserir(arquivoIndiceBin, &cabecalhoAB, estacaoInserir);
    }

    // --- Finalização e Escrita de Cabeçalhos ---
    // Recalcula informações exclusivas do cabeçalho do arquivo de dados (pares/estações)
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoDadosBin, 1);

    // Finaliza os arquivos setando-os como consistentes ('1')
    registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 1, 0);
    arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 1, 0);

    fclose(arquivoDadosBin);
    fclose(arquivoIndiceBin);

    // Output para verificação automática do RunCodes
    BinarioNaTela(nomeArquivoDadosBin);
    BinarioNaTela(nomeArquivoIndiceBin);
}

/* ========================================================================== *
 * FUNCIONALIDADE [10] - DELETE WHERE COM ÁRVORE-B                            *
 * ========================================================================== */

/**
 * @brief Funcionalidade [10]: Deleta registros a partir de um filtro (WHERE). Remoção lógica -> espaço pode ser reaproveitado.
 * @param nomeArquivoDadosBin Nome do arquivo de dados.
 * @param nomeArquivoIndiceBin Nome do arquivo de índice Árvore-B.
 * @param nRemocoes Quantidade de lotes de filtros para remoção a serem executados.
 */
void deleteWhereAB(char *nomeArquivoDadosBin, char *nomeArquivoIndiceBin, int nRemocoes){

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


    // --- Loop remoção ---
    for(int i = 0; i < nRemocoes; i++){

        Busca busca;
        int codEstacao = utils_recebeCampos(&busca);

        // Estratégia 1: Remoção primária indexada (O(log n))
        if(codEstacao != -1) {

                int byteOffset = arvoreb_buscar(arquivoIndiceBin, &cabecalhoAB, codEstacao);

                // Caso a chave exista na árvore
                if(byteOffset != -1) {

                    Registro registro;
                    fseek(arquivoDadosBin, byteOffset, SEEK_SET);
                    registro_lerRegistroBin(arquivoDadosBin, &registro);

                    // Verifica se não está removida e se cumpre eventuais critérios secundários
                    if(registro.removido != '1' && utils_compararRegistroComFiltros(&registro, &busca)){
                        int chaveRemocao = registro.codEstacao;

                        // Remoção no arquivo de Dados
                        registro_deletarRegistro(&registro, &cabecalho, arquivoDadosBin, byteOffset + TAM_REGISTRO);

                        // Remoção na Árvore-B (tratamento de Underflow)
                        arvoreb_remover(arquivoIndiceBin, &cabecalhoAB, chaveRemocao);
                    }
                }
        }
        // Estratégia 2: Remoção secundária via Varredura Sequencial (O(n))
        else {

            // Pula o cabeçalho de 17 bytes
            fseek(arquivoDadosBin, TAM_CABECALHO, SEEK_SET);

            Registro registro;
            int offsetAtual = TAM_CABECALHO;

            while(registro_lerRegistroBin(arquivoDadosBin, &registro) != -1){
                
                offsetAtual += TAM_REGISTRO;

                if(registro.removido == '1') continue;

                // Se o registro for compativel com a busca, remova!
                if(utils_compararRegistroComFiltros(&registro, &busca)){
                    
                    int chaveRemocao = registro.codEstacao;

                    registro_deletarRegistro(&registro, &cabecalho, arquivoDadosBin, offsetAtual);

                    arvoreb_remover(arquivoIndiceBin, &cabecalhoAB, chaveRemocao);
                }
            }
        }

        // Liberação de memória dos campos de busca
        free(busca.campo);
    }

    // --- Finalização ---
    // Recalcula estações/pares ativos na base inteira
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoDadosBin, 1);

    // Restaura consistência ('1') para o próximo uso
    registro_gerenciaCabecalho(&cabecalho, arquivoDadosBin, 1, 0);
    arvoreb_gerenciaCabecalho(&cabecalhoAB, arquivoIndiceBin, 1, 0);

    fclose(arquivoDadosBin);
    fclose(arquivoIndiceBin);

    // Hashing da avaliação automática
    BinarioNaTela(nomeArquivoDadosBin);
    BinarioNaTela(nomeArquivoIndiceBin);
}