#include "funcionalidades.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================== *
 * FUNCIONALIDADE [1] - CREATE TABLE                                          *
 * ========================================================================== */

/**
 * @brief Funcionalidade [1]: Cria um arquivo binário de dados a partir de um CSV.
 * Lê sequencialmente o arquivo texto, converte os tipos de dados, escreve no formato
 * binário de tamanho fixo (80 bytes) e contabiliza estações e pares únicos.
 * * @param nomeArquivoCSV Nome do arquivo de entrada no formato CSV.
 * @param nomeArquivoBin Nome do arquivo de saída binário que será gerado.
 */
void createTable(char *nomeArquivoCSV, char *nomeArquivoBin) {

    // --- Abertura e Validação de Arquivos ---
    FILE *arquivoCSV = fopen(nomeArquivoCSV, "r");
    if(arquivoCSV == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    FILE *arquivoBin = fopen(nomeArquivoBin, "wb");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoCSV);
        return;
    }

    // --- Inicialização e Escrita do Cabeçalho ---
    Cabecalho cabecalho;
    registro_initCabecalho(&cabecalho);
    
    // O status inicial será gravado como '0' (inconsistente) garantindo segurança durante a escrita
    registro_escreverCabecalhoBin(arquivoBin, &cabecalho);

    // --- Processamento das Linhas do CSV ---
    char linha[200];
    Registro registro;

    // Listas encadeadas auxiliares para garantir a unicidade na contagem exigida pelo cabeçalho
    NodeNome *listaNomes = NULL; 
    NodePares *listaPares = NULL; 
    
    // Pula a primeira linha do CSV (nomes das colunas/metadados)
    fgets(linha, sizeof(linha), arquivoCSV); 

    // Loop de extração: lê até encontrar o End Of File (EOF)
    while(fgets(linha, sizeof(linha), arquivoCSV) != NULL) {
        
        // Tratamento de linhas vazias (precaução contra quebras de linha fantasmas no final do arquivo)
        if(linha[0] == '\n' || linha[0] == '\r' || linha[0] == '\0') continue; 

        // Higienização da string: substitui o Carriage Return / Line Feed pelo terminador nulo
        linha[strcspn(linha, "\r\n")] = '\0'; 

        // Fatiamento (tokenização) da linha e formatação da struct de registro
        registro_processaCSV(&registro, linha);

        // Escrita do registro formatado no disco (exatos 80 bytes)
        registro_escreverRegistroBin(arquivoBin, &registro);

        // Atualização de Métricas: Contagem de Estações Únicas
        if(registro.tamNomeEstacao > 0) {
            if(utils_adicionarEstacaoUnica(&listaNomes, registro.nomeEstacao)) 
                cabecalho.nroEstacoes++;
        }

        // Atualização de Métricas: Contagem de Pares (Origem-Destino) Únicos
        if(registro.codProxEstacao != -1) {
            if(utils_adicionarParUnico(&listaPares, registro.codEstacao, registro.codProxEstacao))
                cabecalho.nroParesEstacao++;
        }
        
        // Atualiza a posição do próximo registro a ser inserido no final do arquivo
        cabecalho.proxRRN++; 
    }

    // --- Finalização ---
    // Atualiza a flag de consistência para '1' (estável) e regrava o cabeçalho finalizado
    registro_gerenciaCabecalho(&cabecalho, arquivoBin, 1, 0);

    fclose(arquivoCSV);
    fclose(arquivoBin);
    
    // Libera a memória dinâmica alocada pelas estruturas auxiliares de contagem
    utils_liberarUtils(listaNomes, listaPares); 

    BinarioNaTela(nomeArquivoBin);
}


/* ========================================================================== *
 * FUNCIONALIDADE [2] - SELECT FROM                                           *
 * ========================================================================== */

/**
 * @brief Funcionalidade [2]: Lê e imprime todos os registros válidos do arquivo binário.
 * Registros marcados logicamente como removidos ('1') são ignorados na impressão.
 * * @param nomeArquivoBin Nome do arquivo binário de dados.
 */
void selectFromTable(char *nomeArquivoBin) {

    // --- Abertura e Validação de Arquivos ---
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb");
    if (arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // --- Verificação de Consistência ---
    Cabecalho cabecalho; 
    fread(&cabecalho.status, sizeof(char), 1, arquivoBin);
    if (!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 1))
        return;
    
    // Posiciona o cursor de leitura do disco após os 17 bytes iniciais (Cabeçalho)
    fseek(arquivoBin, 17, SEEK_SET); 

    // --- Leitura e Impressão Sequencial ---
    Registro registro;
    int statusLeitura;
    int existeRegistro = 0; // Flag para controlar se a base de dados está completamente vazia

    // O loop avança de 80 em 80 bytes até o final do arquivo
    while ((statusLeitura = registro_lerRegistroBin(arquivoBin, &registro)) != -1) {
        // Status 1 indica que o registro foi lido com sucesso e NÃO está logicamente removido
        if (statusLeitura == 1) {
            registro_imprimirRegistro(&registro);
            existeRegistro = 1;
        }
    }

    // Feedback exigido pela especificação caso nenhum registro válido seja encontrado
    if (!existeRegistro) {
        printf("Registro inexistente.\n");
    }

    fclose(arquivoBin);
}


/* ========================================================================== *
 * FUNCIONALIDADE [3] - SELECT FROM WHERE                                     *
 * ========================================================================== */

/**
 * @brief Funcionalidade [3]: Recupera registros que satisfazem parâmetros de busca específicos.
 * O arquivo binário já deve vir aberto, pois esta função pode ser chamada como fallback
 * por outras rotinas maiores (ex: Funcionalidade 8).
 * * @param arquivoBin Ponteiro do descritor de arquivo aberto.
 * @param busca Estrutura contendo os filtros exigidos pelo usuário.
 */
void selectWhere(FILE *arquivoBin, Busca *busca) {

    // O arquivo de dados fornecido já está aberto.
    // Não faremos verificação de consistência do arquivo, 
    // pois já fizemos na função selecWhereAB, que chamou esta.

    // Posiciona o cursor para o primeiro registro de dados
    fseek(arquivoBin, 17, SEEK_SET); 
        
    Registro registro;
    int encontrouRegistro = 0;

    // --- Varredura Linear de Busca (O(n)) ---
    while (registro_lerRegistroBin(arquivoBin, &registro) != -1) {
        
        // Registros logicamente apagados não devem participar da busca.
        // A função de leitura faz a checagem preliminar; ela só lê o registro 
        // inteiro se ele não estiver com o bit de removido setado.
        if (registro.removido == '1') continue; 

        // Avalia se as propriedades do registro dão 'match' completo com as chaves de busca
        if (utils_compararRegistroComFiltros(&registro, busca)) {
            registro_imprimirRegistro(&registro);
            encontrouRegistro = 1;
        }
    }

    if (!encontrouRegistro) 
        printf("Registro inexistente.\n");

    // Formatação da saída para delimitar encerramento da operação
    printf("\n");
    
    // Limpeza mandatória para evitar vazamento de memória do array de filtros
    free(busca->campo); 
}


/* ========================================================================== *
 * FUNCIONALIDADE [4] - DELETE WHERE                                          *
 * ========================================================================== */

/**
 * @brief Funcionalidade [4]: Remove logicamente registros que atendem aos filtros especificados.
 * A remoção lógica marca o registro como inativo e o insere em uma pilha de reaproveitamento de espaço.
 * * @param nomeArquivoBin Nome do arquivo de dados que será modificado.
 * @param nRemocoes Número de operações de exclusão sequenciais a serem executadas.
 */
void deleteWhere(char *nomeArquivoBin, int nRemocoes) {

    // --- Abertura em Modo Misto (Leitura e Atualização Binária) ---
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // --- Carregamento de Cabeçalho e Marcação de Inconsistência ---
    Cabecalho cabecalho;
    registro_lerCabecalho(arquivoBin, &cabecalho);
    
    // A função marca status = '0' no disco pois haverá escrita na estrutura
    if (!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 0))
        return;
    
    // Executa as baterias de deleção solicitadas
    for (int i = 0; i < nRemocoes; i++) {

        // Extrai os filtros da STDIN (teclado/casos de teste)
        Busca busca;
        utils_recebeCampos(&busca);
            
        // Reseta o cursor de leitura para o topo dos registros a cada nova busca
        fseek(arquivoBin, 17, SEEK_SET); 
        
        Registro registro;
        int offsetAtual = TAM_CABECALHO;

        // Varredura do registro
        while (registro_lerRegistroBin(arquivoBin, &registro) != -1) {
            
            // Seguimos em frente para o próximo registro
            offsetAtual += TAM_REGISTRO;

            if (registro.removido == '1') continue; 

            // Se for um alvo, aplica a deleção física da chave (tombstone) e gerencia a lista invertida
            if (utils_compararRegistroComFiltros(&registro, &busca)) 
                registro_deletarRegistro(&registro, &cabecalho, arquivoBin, offsetAtual);
            
            
        }
        
        // Descarta filtros processados para liberar a memória Heap
        free(busca.campo);
    }

    // --- Manutenção Estrutural ---
    // Recalcula contadores do cabeçalho que podem ter sido afetados pelas deleções
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoBin, 1);
    
    // Uma vez que as deleções finalizaram, gravamos as alterações com status consistente (1)
    registro_gerenciaCabecalho(&cabecalho, arquivoBin, 1, 0);
    
    fclose(arquivoBin);

    BinarioNaTela(nomeArquivoBin);
}


/* ========================================================================== *
 * FUNCIONALIDADE [5] - INSERT INTO                                           *
 * ========================================================================== */

/**
 * @brief Funcionalidade [5]: Insere novos registros na base de dados.
 * Otimiza o armazenamento consumindo (em O(1)) RRNs do topo da pilha de removidos
 * antes de expandir o arquivo (EOF).
 * * @param nomeArquivoBin Nome do arquivo binário.
 * @param nInsercoes Quantidade de registros a serem inseridos.
 */
void insertInto(char *nomeArquivoBin, int nInsercoes) {

    // --- Abertura Mista ---
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    Cabecalho cabecalho;
    registro_lerCabecalho(arquivoBin, &cabecalho);
    
    // Flag de inconsistência acionada preventivamente
    if(!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 0))
        return;
    
    // Processamento das requisições
    for (int i = 0; i < nInsercoes; i++) {

        // Parse e carregamento do novo registro para a RAM
        Registro registro;
        registro_lerRegistro(&registro);

        // --- Estratégia de Fragmentação: Reutilização da Pilha (Worst-Fit Lógico) ---
        if (cabecalho.topo != -1) {

            // Salto físico para o offset absoluto do nó removido
            fseek(arquivoBin, TAM_CABECALHO + cabecalho.topo * TAM_REGISTRO + 1, SEEK_SET);
            
            // O RRN que assumirá a cabeça da pilha foi guardado nos 4 bytes subsequentes
            fread(&cabecalho.topo, sizeof(int), 1, arquivoBin);

            // Retorna o cursor para o byte incial do registro (status de remoção) para sobreposição total
            fseek(arquivoBin, - sizeof(int) - 1, SEEK_CUR);
            registro_escreverRegistroBin(arquivoBin, &registro);
        }
        // --- Estratégia de Fragmentação: Append Sequencial (Fim do Arquivo) ---
        else {

            // Posicionamento direto no RRN virgem e atualização de métrica de controle
            fseek(arquivoBin, TAM_CABECALHO + cabecalho.proxRRN * TAM_REGISTRO, SEEK_SET);
            registro_escreverRegistroBin(arquivoBin, &registro);
            cabecalho.proxRRN++;        
        }
    }

    // Varredura para reestabelecer as restrições globais de unicidade para o cabeçalho
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoBin, 0);

    // Commit da transação setando o sistema para consistente novamente
    registro_gerenciaCabecalho(&cabecalho, arquivoBin, 1, 0);

    fclose(arquivoBin);

    BinarioNaTela(nomeArquivoBin);
}


/* ========================================================================== *
 * FUNCIONALIDADE [6] - UPDATE                                                *
 * ========================================================================== */

/**
 * @brief Funcionalidade [6]: Atualiza campos específicos de registros existentes.
 * Localiza os alvos usando os critérios de busca (filtros) e sobrescreve apenas os 
 * campos fornecidos sem desestruturar o resto do registro.
 * * @param nomeArquivoBin Nome do arquivo alvo de atualização.
 * @param nAtualizacoes Número de operações de 'Update' a serem realizadas.
 */
void update(char* nomeArquivoBin, int nAtualizacoes) {

    // --- Abertura Mista ---
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // Leitura econômica do cabeçalho (somente o status interessa para avaliar a segurança)
    Cabecalho cabecalho;
    fread(&cabecalho.status, sizeof(char), 1, arquivoBin);
    
    // Trava de segurança ativada no disco ('0')
    if(!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 0))
        return;

    // O laço avança de 2 em 2 pois a STDIN fornecerá primeiro os filtros da busca
    // e logo na linha seguinte os valores que serão atualizados.
    for (int i = 0; i < 2*nAtualizacoes; i += 2) {

        // Desempacotamento de pares: [Filtros -> Cláusula WHERE] e [Valores -> Cláusula SET]
        Busca filtros, valores;
        utils_recebeCampos(&filtros);
        utils_recebeCampos(&valores);
        
        // Posição zero dos dados
        fseek(arquivoBin, 17, SEEK_SET);
        
        Registro registro;
        int offsetAtual = TAM_CABECALHO;

        // Varredura Sequencial
        while (registro_lerRegistroBin(arquivoBin, &registro) != -1) {
            
            // Seguimos em frente para o próximo registro
            offsetAtual += TAM_REGISTRO;
            
            if (registro.removido == '1') continue; 

            // Confirmação de alvo para aplicação física do update no disco
            if (utils_compararRegistroComFiltros(&registro, &filtros)) {
                utils_atualizarRegistroComFiltros(valores, arquivoBin, offsetAtual);
            }

        }
        
        // Liberação dos vetores de ponteiros
        free(filtros.campo); 
        free(valores.campo); 
    }

    // --- Finalização Otimizada ---
    // O arquivo será fechado com status consistente.
    // Não usamos a função "registro_gerenciaCabecalho" completa aqui pois evitamos o custo
    // de I/O de reescrever todas as outras variáveis (como topo e nroPares) que não sofreram mutação.
    cabecalho.status = '1';
    fseek(arquivoBin, 0, SEEK_SET);
    fwrite(&cabecalho.status, sizeof(char), 1, arquivoBin);

    fclose(arquivoBin);

    BinarioNaTela(nomeArquivoBin);
}