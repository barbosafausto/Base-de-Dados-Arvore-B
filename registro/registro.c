#include "registro.h"

/* ========================================================================== *
 * PROCESSAMENTO DE DADOS (CSV E ENTRADA PADRÃO)                              *
 * ========================================================================== */

/**
 * @brief Processa uma linha extraída de um arquivo CSV e a converte para a estrutura Registro.
 * Lida com campos nulos (vazios entre vírgulas) convertendo-os para -1 ou tamanho 0.
 * @param registro Ponteiro para a estrutura que armazenará os dados formatados.
 * @param linha String contendo a linha bruta do CSV lida via fgets.
 */
void registro_processaCSV(Registro *registro, char linha[]) {

        // Ponteiro de char que será processado via strsep para fatiar a string
        char* pLinha = linha;

        // Token que armazenará cada campo isolado no processamento
        char* token;

        // Inicialização de metadados padrão do registro
        registro->removido = '0'; // '0' indica que o registro é válido (não removido)
        registro->proximo = -1;   // -1 indica que não aponta para ninguém na pilha de remoção

        // Campo 1: codEstacao (Inteiro)
        token = strsep(&pLinha, ",");
        registro->codEstacao = atoi(token);

        // Campo 2: nomeEstacao (String de tamanho variável)
        token = strsep(&pLinha, ",");
        registro->tamNomeEstacao = strlen(token);
        strcpy(registro->nomeEstacao, token);

        // Campo 3: codLinha (Inteiro que pode ser Nulo)
        token = strsep(&pLinha, ",");
        registro->codLinha = (token[0] == '\0') ? -1 : atoi(token);

        // Campo 4: nomeLinha (String de tamanho variável que pode ser Nula)
        token = strsep(&pLinha, ",");
        if (token[0] == '\0') registro->tamNomeLinha = 0; 
        else {
            registro->tamNomeLinha = strlen(token);
            strcpy(registro->nomeLinha, token);
        }

        // Campo 5: codProxEstacao (Inteiro que pode ser Nulo)
        token = strsep(&pLinha, ",");
        registro->codProxEstacao = (token[0] == '\0') ? -1 : atoi(token);

        // Campo 6: distProxEstacao (Inteiro que pode ser Nulo)
        token = strsep(&pLinha, ",");
        registro->distProxEstacao = (token[0] == '\0') ? -1 : atoi(token);

        // Campo 7: codLinhaIntegra (Inteiro que pode ser Nulo)
        token = strsep(&pLinha, ",");
        registro->codLinhaIntegra = (token[0] == '\0') ? -1 : atoi(token);

        // Campo 8: codEstIntegra (Inteiro que pode ser Nulo)
        token = strsep(&pLinha, ",");
        registro->codEstIntegra = (token[0] == '\0') ? -1 : atoi(token);
}

/**
 * @brief Lê os campos de um registro via entrada padrão (teclado/stdin).
 * Utilizado primariamente na funcionalidade INSERT INTO (5).
 * @param registro Ponteiro para a estrutura que será preenchida.
 */
void registro_lerRegistro(Registro *registro) {

    // 100 bytes: comprimento seguro de buffer, visto que o registro físico tem 80 bytes
    char valor[85];
    int valorInt; 

    // Inicialização padrão para novos registros inseridos
    registro->removido = '0';
    registro->proximo = -1;
    
    // O laço itera exatamente sobre as 8 colunas que compõem o modelo de dados
    for (int j = 0; j < 8; j++) { 

        // Leitura de Strings (com suporte a aspas duplas, via função fornecida)
        if (j == 1 || j == 3) 
            ScanQuoteString(valor);

        // Leitura de Inteiros (com suporte ao tratamento da string "NULO")
        else {
            scanf(" %s", valor);
            valorInt = (strcmp(valor, "NULO") == 0) ? -1 : atoi(valor);
        }
        
        // Distribuição dos dados armazenados no buffer para os campos corretos da struct
        if (j == 0)      registro->codEstacao = valorInt;

        else if (j == 1) {
            strcpy(registro->nomeEstacao, valor);
            registro->tamNomeEstacao = strlen(valor);
        }
        else if (j == 2) registro->codLinha = valorInt;

        else if (j == 3) {
            strcpy(registro->nomeLinha, valor);
            registro->tamNomeLinha = strlen(valor);
        }

        else if (j == 4) registro->codProxEstacao = valorInt;
        else if (j == 5) registro->distProxEstacao = valorInt;
        else if (j == 6) registro->codLinhaIntegra = valorInt;
        else             registro->codEstIntegra = valorInt;
    }
}


/* ========================================================================== *
 * GERENCIAMENTO DO CABEÇALHO (I/O E ESTADO)                                  *
 * ========================================================================== */

/**
 * @brief Inicializa a estrutura de cabeçalho na RAM com valores zerados/padrão.
 * @param cabecalho Ponteiro para a estrutura de cabeçalho.
 */
void registro_initCabecalho(Cabecalho *cabecalho) {
    cabecalho->status = '0';            // Inconsistente (padrão durante manipulação)
    cabecalho->topo = -1;               // Nenhum registro removido (Pilha vazia inicial)
    cabecalho->proxRRN = 0;             // Próximo RRN disponível é o offset zero dos dados
    cabecalho->nroEstacoes = 0;         // Contador zerado
    cabecalho->nroParesEstacao = 0;     // Contador zerado
}

/**
 * @brief Lê sequencialmente os 17 bytes de metadados do topo do arquivo binário.
 * @param arquivoBin Arquivo binário de onde o cabeçalho será lido.
 * @param cabecalho Estrutura na RAM que receberá os dados.
 */
void registro_lerCabecalho(FILE *arquivoBin, Cabecalho *cabecalho) {
    fread(&cabecalho->status,          1, 1, arquivoBin);
    fread(&cabecalho->topo,            4, 1, arquivoBin);
    fread(&cabecalho->proxRRN,         4, 1, arquivoBin);
    fread(&cabecalho->nroEstacoes,     4, 1, arquivoBin);
    fread(&cabecalho->nroParesEstacao, 4, 1, arquivoBin);
}

/**
 * @brief Escreve os 17 bytes da estrutura do cabeçalho no arquivo binário de dados.
 * Assume que o ponteiro do arquivo (fseek) já foi posicionado corretamente no byte 0.
 * @param arquivo Descritor do arquivo binário.
 * @param cabecalho Ponteiro contendo os dados prontos.
 */
void registro_escreverCabecalhoBin(FILE *arquivo, Cabecalho *cabecalho) {
    // Escrever o cabeçalho no arquivo binário (Exatos 17 bytes totais)
    // Evita-se sizeof() para estruturas pois o padding do C poderia distorcer o tamanho físico.
    fwrite(&cabecalho->status,          1, 1, arquivo);
    fwrite(&cabecalho->topo,            4, 1, arquivo);
    fwrite(&cabecalho->proxRRN,         4, 1, arquivo);
    fwrite(&cabecalho->nroEstacoes,     4, 1, arquivo);
    fwrite(&cabecalho->nroParesEstacao, 4, 1, arquivo);
}

/**
 * @brief Controla de forma inteligente a consistência ('0' ou '1') do arquivo binário.
 * @param cabecalho Ponteiro para a estrutura do cabeçalho.
 * @param arquivoBin Descritor do arquivo.
 * @param escreverConsistente Flag (1: fecha arquivo setando consistente; 0: verifica ou seta inconsistência).
 * @param leitura Flag (1: aberto para leitura "rb"; 0: aberto para atualização "rb+" ou "wb").
 * @return int Retorna 1 em caso de sucesso/aprovação de acesso, ou 0 em caso de arquivo corrompido/inconsistente.
 */
int registro_gerenciaCabecalho(Cabecalho *cabecalho, FILE *arquivoBin, int escreverConsistente, int leitura) {
    
    // Caso em que estamos abrindo e preparando o arquivo para a funcionalidade
    if (escreverConsistente == 0) {

        // Proteção: Se o status no disco for '0' antes de sequer começarmos, o arquivo crashou em uma execução anterior.
        if (cabecalho->status == '0') { 
            printf("Falha no processamento do arquivo.\n");
            fclose(arquivoBin);
            return 0;
        }

        // Se a funcionalidade exigirá escrita (INSERT, UPDATE, DELETE, etc.)
        if (leitura == 0) {
            // Seta preventivamente o disco para inconsistente ('0') para proteção de quebra de energia.
            cabecalho->status = '0';
            fseek(arquivoBin, 0, SEEK_SET);
            fwrite(&cabecalho->status, sizeof(char), 1, arquivoBin); 
        }
    }
    // Caso em que a funcionalidade terminou com sucesso e precisamos selar o arquivo
    else {
        cabecalho->status = '1'; 
        fseek(arquivoBin, 0, SEEK_SET);
        registro_escreverCabecalhoBin(arquivoBin, cabecalho); // Salva as métricas junto com o status '1'
    }

    return 1;
}


/* ========================================================================== *
 * MANIPULAÇÃO DE REGISTROS (I/O, IMPRESSÃO E DELEÇÃO)                        *
 * ========================================================================== */

/**
 * @brief Lê um registro de dados do arquivo binário para a memória principal.
 * Lida com o tratamento de tamanho fixo do disco (80 bytes) e descarta o lixo ('$').
 * @param arquivo Descritor do arquivo binário posicionado no registro alvo.
 * @param registro Ponteiro para a estrutura que será preenchida.
 * @return int 1 (Lido com sucesso), 0 (Registro pulado por estar logicamente removido), -1 (EOF alcançado).
 */
int registro_lerRegistroBin(FILE *arquivo, Registro *registro) {
    
    // Tenta extrair o byte inicial. Se falhar, é o Fim do Arquivo (EOF).
    if(fread(&registro->removido, sizeof(char), 1, arquivo) != 1) 
        return -1;
    
    // Se o registro possuir uma tombstone ('1'), ignoramos a extração para poupar processamento
    // Avançamos fisicamente os 79 bytes restantes deste registro.
    if(registro->removido == '1') {
        fseek(arquivo, 79, SEEK_CUR);
        return 0;
    }

    // Leitura dos campos de tamanho fixo estático
    fread(&registro->proximo,         sizeof(int), 1, arquivo);
    fread(&registro->codEstacao,      sizeof(int), 1, arquivo);
    fread(&registro->codLinha,        sizeof(int), 1, arquivo);
    fread(&registro->codProxEstacao,  sizeof(int), 1, arquivo);
    fread(&registro->distProxEstacao, sizeof(int), 1, arquivo);
    fread(&registro->codLinhaIntegra, sizeof(int), 1, arquivo);
    fread(&registro->codEstIntegra,   sizeof(int), 1, arquivo);

    // Leitura do campo de tamanho dinâmico: nomeEstacao
    fread(&registro->tamNomeEstacao, sizeof(int), 1, arquivo);
    if (registro->tamNomeEstacao > 0) {
        fread(registro->nomeEstacao, sizeof(char), registro->tamNomeEstacao, arquivo);
        registro->nomeEstacao[registro->tamNomeEstacao] = '\0'; // Terminador de string manual
    } 
    else {
        registro->nomeEstacao[0] = '\0'; // Campo vazio
    }

    // Leitura do campo de tamanho dinâmico: nomeLinha
    fread(&registro->tamNomeLinha, sizeof(int), 1, arquivo);
    if (registro->tamNomeLinha > 0) {
        fread(registro->nomeLinha, sizeof(char), registro->tamNomeLinha, arquivo);
        registro->nomeLinha[registro->tamNomeLinha] = '\0'; // Terminador de string manual
    } 
    else {
        registro->nomeLinha[0] = '\0'; // Campo vazio
    }

    // Calcula os bytes consumidos na leitura e salta o lixo de preenchimento do disco ('$')
    int bytesLidos = 37 + registro->tamNomeEstacao + registro->tamNomeLinha;
    if(bytesLidos < TAM_REGISTRO) {
        fseek(arquivo, TAM_REGISTRO - bytesLidos, SEEK_CUR);
    }

    return 1; // Sucesso
}

/**
 * @brief Grava a estrutura da RAM para o disco, padronizando estritamente em 80 bytes.
 * @param arquivo Descritor do arquivo binário posicionado na área de gravação.
 * @param registro Registro contendo os dados formatados.
 */
void registro_escreverRegistroBin(FILE *arquivo, Registro *registro) {
    
    // Escrita sequencial dos campos fixos
    fwrite(&registro->removido,        sizeof(char), 1, arquivo);
    fwrite(&registro->proximo,         sizeof(int), 1, arquivo);
    fwrite(&registro->codEstacao,      sizeof(int), 1, arquivo);
    fwrite(&registro->codLinha,        sizeof(int), 1, arquivo);
    fwrite(&registro->codProxEstacao,  sizeof(int), 1, arquivo);
    fwrite(&registro->distProxEstacao, sizeof(int), 1, arquivo);
    fwrite(&registro->codLinhaIntegra, sizeof(int), 1, arquivo);
    fwrite(&registro->codEstIntegra,   sizeof(int), 1, arquivo);

    // Escrita condicional dos campos variáveis
    fwrite(&registro->tamNomeEstacao, sizeof(int), 1, arquivo);
    if(registro->tamNomeEstacao > 0) 
        fwrite(registro->nomeEstacao, sizeof(char), registro->tamNomeEstacao, arquivo);

    fwrite(&registro->tamNomeLinha, sizeof(int), 1, arquivo);
    if(registro->tamNomeLinha > 0) 
        fwrite(registro->nomeLinha, sizeof(char), registro->tamNomeLinha, arquivo);

    // Preenchimento de padronização (Padding) com o caractere '$' (definido em LIXO)
    // Isso garante que todo registro ocupará sempre o tamanho matemático de TAM_REGISTRO (80 bytes)
    int bytesEscritos = 37 + registro->tamNomeEstacao + registro->tamNomeLinha;
    while(bytesEscritos < TAM_REGISTRO) {
        fputc(LIXO, arquivo);
        bytesEscritos++;
    }
}

/**
 * @brief Exibe na saída padrão um registro de dados formatado de acordo com as especificações.
 * Converte matematicamente os marcadores de nulo (-1 ou 0) na string literal "NULO".
 * @param registro O registro populado com dados a ser impresso.
 */
void registro_imprimirRegistro(Registro *registro) {
    
    // Campos not-null (nunca serão inválidos pela especificação)
    printf("%d ", registro->codEstacao);
    printf("%s ", registro->nomeEstacao);

    // Campos nullables (avaliação ternária para formatação fluida)
    (registro->codLinha == -1)          ? printf("NULO ")  : printf("%d ",  registro->codLinha);
    (registro->tamNomeLinha == 0)       ? printf("NULO ")  : printf("%s ",  registro->nomeLinha);
    (registro->codProxEstacao == -1)    ? printf("NULO ")  : printf("%d ",  registro->codProxEstacao);
    (registro->distProxEstacao == -1)   ? printf("NULO ")  : printf("%d ",  registro->distProxEstacao);
    (registro->codLinhaIntegra == -1)   ? printf("NULO ")  : printf("%d ",  registro->codLinhaIntegra);
    (registro->codEstIntegra == -1)     ? printf("NULO\n") : printf("%d\n", registro->codEstIntegra);
}

/**
 * @brief Marca fisicamente um registro como apagado e o insere no topo da pilha do cabeçalho.
 * Aproveita o offset de leitura avançado (offsetAtual) para retornar e alterar exatamente 5 bytes ('removido' e 'proximo').
 * @param registro Estrutura alvo.
 * @param cabecalho Cabeçalho mantido na RAM que dita o antigo topo da pilha.
 * @param arquivoBin Descritor de arquivo aberto.
 * @param offsetAtual Cursor físico atual no arquivo (apontando para o final deste registro).
 */
void registro_deletarRegistro(Registro *registro, Cabecalho *cabecalho, FILE *arquivoBin, int offsetAtual) {

    // O pulo do gato logístico: o offsetAtual marca onde o laço while principal parou de ler (início do próximo registro).
    // Recuamos exatos 80 bytes para focar no primeiro byte do registro que originou a leitura.
    int byteOffsetRegistro = offsetAtual - 80;

    // Atualização dos dados estruturais do registro em memória
    registro->removido = '1';                 
    registro->proximo = cabecalho->topo;      // Link do LIFO: O "próximo" aponta para o antigo topo da pilha

    // Posicionamento do cursor para o início do registro a ser morto
    fseek(arquivoBin, byteOffsetRegistro, SEEK_SET);

    // Update de informações cirúrgico no disco (sobrescrevendo apenas os primeiros 5 bytes)
    fwrite(&registro->removido, sizeof(char), 1, arquivoBin);
    fwrite(&registro->proximo, sizeof(int), 1, arquivoBin);

    // Atualização do RRN do topo da pilha no cabeçalho
    // (A variável no arquivo será persistida fisicamente apenas quando as deleções encerrarem)
    cabecalho->topo = byteOffsetRegistro / 80;

    // Retorna a cabeça de leitura/gravação magnética para o final do registro atual (offsetAtual).
    // Isso garante que o laço exterior `while(registro_lerRegistroBin...)` não perca o compasso matemático 
    // e consiga ler o próximo registro sem falhas de sincronia.
    fseek(arquivoBin, offsetAtual, SEEK_SET);
}