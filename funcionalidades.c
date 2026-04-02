#include "funcionalidades.h"
#include "registro.h"
#include "utils.h"
#include "fornecidas.h" // Dos monitores

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Funcionalidade [1] - Create Table
void createTable(char *nomeArquivoCSV, char *nomeArquivoBin) {
    // Implementação da funcionalidade de criação da tabela a partir do CSV [1]

    // Abrir arquivos
    FILE *arquivoCSV = fopen(nomeArquivoCSV, "r");
    if(arquivoCSV == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    FILE *arquivoBin = fopen(nomeArquivoBin, "wb");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }


    // Inicializar o cabeçalho como inconsistente
    Cabecalho cabecalho;
    initCabecalho(&cabecalho);
    //cabecalho->status = '0';   // Já é definido como '0' na função initCabecalho

    // Escrever o cabeçalho no arquivo binário (17 bytes)
    escreverCabecalhoBin(arquivoBin, &cabecalho);

    
    // Processamento das linhas do CSV
    char linha[1024];
    Registro registro;
    
    NodeNome *listaNomes = NULL; // Lista para armazenar estações já vistas
    NodePares *listaPares = NULL; // Lista para armazenar pares de estações já vistos
    
    
    fgets(linha, sizeof(linha), arquivoCSV); // Pular a linha de cabeçalho do CSV
    while(fgets(linha, sizeof(linha), arquivoCSV) != NULL) {

        if(linha[0] == '\n' || linha[0] == '\r' || linha[0] == '\0') continue; // Linha vazia
        // memset(&registro, 0, sizeof(Registro));

        linha[strcspn(linha, "\r\n")] = '\0'; // Remover o caractere de nova linha

        char *pLinha = linha;
        char *token;

        registro.removido = '0'; // Registro válido
        registro.proximo = -1;   // Não há próximo registro removido

        // codEstacao
        token = strsep(&pLinha, ",");
        registro.codEstacao = atoi(token);

        // nomeEstacao
        token = strsep(&pLinha, ",");
        registro.tamNomeEstacao = strlen(token);
        strcpy(registro.nomeEstacao, token);

        // codLinha - int
        token = strsep(&pLinha, ",");
        if(token[0] == '\0') {
            registro.codLinha = -1; // Valor padrão para campo ausente
        } else {
            registro.codLinha = atoi(token);
        }

        // nomeLinha - string
        token = strsep(&pLinha, ",");
        if(token[0] == '\0') {
            registro.tamNomeLinha = 0; // Campo ausente
        } else {
            registro.tamNomeLinha = strlen(token);
            strcpy(registro.nomeLinha, token);
        }

        // Usando operador ternário p melhorar visualização
        // Fazem a mesma coisa que o bloco if-else do codLinha

        // codProxEstacao (int)
        token = strsep(&pLinha, ",");
        registro.codProxEstacao = (token[0] == '\0') ? -1 : atoi(token);

        // distProxEstacao (int)
        token = strsep(&pLinha, ",");
        registro.distProxEstacao = (token[0] == '\0') ? -1 : atoi(token);

        // codLinhaIntegra (int)
        token = strsep(&pLinha, ",");
        registro.codLinhaIntegra = (token[0] == '\0') ? -1 : atoi(token);

        // codEstIntegra (int)
        token = strsep(&pLinha, ",");
        registro.codEstIntegra = (token[0] == '\0') ? -1 : atoi(token);


        // Escrever o registro no arquivo binário
        escreverRegistroBin(arquivoBin, &registro);

        if(registro.tamNomeEstacao > 0) {
            if(adicionarEstacaoUnica(&listaNomes, registro.nomeEstacao)) 
                cabecalho.nroEstacoes++;
        }

        if(registro.codProxEstacao != -1) {
            if(adicionarParUnico(&listaPares, registro.codEstacao, registro.codProxEstacao))
                cabecalho.nroParesEstacao++;
        }
        
        cabecalho.proxRRN++; // Incrementar o próximo RRN disponível

    }

    // Finalizar Create Table
    cabecalho.status = '1'; // Tabela consistente, conforme especificação
    // debbug
    //printf("Status: %c, Topo: %d, ProxRRN: %d, nroEstacoes: %d, nroParesEstacao: %d\n", cabecalho.status, cabecalho.topo, cabecalho.proxRRN, cabecalho.nroEstacoes, cabecalho.nroParesEstacao);

    fseek(arquivoBin, 0, SEEK_SET); // Voltar para o início do arquivo para atualizar o cabeçalho
    escreverCabecalhoBin(arquivoBin, &cabecalho); // Atualizar o cabeçalho com os valores finais

    fclose(arquivoCSV);
    fclose(arquivoBin);

    liberarUtils(listaNomes, listaPares); // Liberar memória das listas
    BinarioNaTela(nomeArquivoBin);
}

// Funcionalidade [2] - Select
void selectFromTable(char *nomeArquivoBin) {
    // Implementação da funcionalidade Select [2]

    FILE *arquivoBin = fopen(nomeArquivoBin, "rb");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    char status;
    fread(&status, sizeof(char), 1, arquivoBin);

    if(status == '0') { // inconsistente
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoBin);
        return;
    }

    fseek(arquivoBin, 17, SEEK_SET); // Pular cabeçalho

    Registro registro;
    int statusLeitura;
    int existeRegistro = 0;

    while((statusLeitura = lerRegistroBin(arquivoBin, &registro)) != -1) {
        if(statusLeitura == 1) {
            imprimirRegistro(&registro);
            existeRegistro = 1;
        }
    }

    if(!existeRegistro) {
        printf("Registro inexistente.\n");
    }

    fclose(arquivoBin);
}

// Funcionalidade [3] - Where
void selectWhere(char *nomeArquivoBin, int nBuscas) {
    // Implementação da funcionalidade Select Where [3]

    // Leitura dos filtros
    Busca *busca = (Busca *) malloc(nBuscas * sizeof(Busca));
    recebeFiltros(busca, nBuscas);

    // Processamento das buscas
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    char status;
    fread(&status, sizeof(char), 1, arquivoBin);

    if(status == '0') { // inconsistente
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoBin);
        return;
    }

    for (int i = 0; i < nBuscas; i++) {
        fseek(arquivoBin, 17, SEEK_SET); // Pular cabeçalho
        
        Registro registro;
        int encontrouRegistro = 0;

        while (lerRegistroBin(arquivoBin, &registro) != -1) {
            if (registro.removido == '1') continue; // Pular registros removidos

            if (compararRegistroComFiltros(&registro, &busca[i])) {
                imprimirRegistro(&registro);
                encontrouRegistro = 1;
            }
        }

        if (!encontrouRegistro) {
            printf("Registro inexistente.");
            printf("\n");   // Quebra de linha para alinhar com o runcodes 
        }

        // Formatação da saída
        printf("\n");

        free(busca[i].filtro); // Liberar memória dos filtros da busca atual
    }

    free(busca); // Liberar memória da busca
    fclose(arquivoBin);
}

// Funcionalidade [4] - Delete
void deleteWhere(char *nomeArquivoBin, int nRemocoes) {

    //Vetor que vai guardar os filtros da busca
    Busca *busca = (Busca*) malloc(nRemocoes * sizeof(Busca));

    recebeFiltros(busca, nRemocoes);

    // Processamento das deleções
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    Cabecalho cabecalho;
    lerCabecalho(&cabecalho, arquivoBin);

    //printf("Status lido: '%c' | Topo: %d | RRN: %d | Estacoes: %d\n", 
    //   cabecalho.status, cabecalho.topo, cabecalho.proxRRN, cabecalho.nroEstacoes); 


    // Verificando consistência do arquivo
    if(cabecalho.status == '0') { // inconsistente
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoBin);
        return;
    }

    // O arquivo foi aberto para leitura: status deve ser 0
    cabecalho.status = '0';
    

    for (int i = 0; i < nRemocoes; i++) {
        fseek(arquivoBin, 17, SEEK_SET); // Pular cabeçalho
        
        Registro registro;

        int offsetAtual = 17;
        while (lerRegistroBin(arquivoBin, &registro) != -1) {
            if (registro.removido == '1') continue; // Pular registros removidos

            offsetAtual += 80;
            if (compararRegistroComFiltros(&registro, &busca[i])) {
                deletarRegistro(&registro, &cabecalho, arquivoBin, offsetAtual);
            }
        }

        free(busca[i].filtro); // Liberar memória dos filtros da busca atual
    }

    // Agora, é necessário recontar o número de estações para ver se ainda temos o mesmo número ou não
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoBin);

    // Uma vez que as deleções finalizaram, podemos escrever o novo cabeçalho
    cabecalho.status = '1'; // O arquivo será fechado: status = 1;

    //printf("Status lido: '%c' | Topo: %d | RRN: %d | Estacoes: %d\n | Pares: %d\n", 
    //   cabecalho.status, cabecalho.topo, cabecalho.proxRRN, cabecalho.nroEstacoes, cabecalho.nroParesEstacao); 
   
    fseek(arquivoBin, 0, SEEK_SET);
    escreverCabecalhoBin(arquivoBin, &cabecalho);

    free(busca); // Liberar memória da busca
    fclose(arquivoBin);

    BinarioNaTela(nomeArquivoBin);
}

// Funcionalidade [5] - Insert
void insertInto(char *nomeArquivoBin, int nInsercoes) {

    // Lista de registros que 
    Registro *registros = utils_leRegistros(nInsercoes);

    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    Cabecalho cabecalho;
    lerCabecalho(&cabecalho, arquivoBin);
    
    // Verificando consistência do arquivo
    if(cabecalho.status == '0') { // inconsistente
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoBin);
        return;
    }

    //Arquivo aberto: inconsistente
    cabecalho.status = '0';

    for (int i = 0; i < nInsercoes; i++) {

        //Inserção no topo da pilha
        if (cabecalho.topo != -1) {

            //Atualização do topo da pilha
            fseek(arquivoBin, 17 + cabecalho.topo * 80 + 1, SEEK_SET);
            fread(&cabecalho.topo, sizeof(int), 1, arquivoBin);

            //Inserção do novo registro
            fseek(arquivoBin, - sizeof(int), SEEK_CUR);
            escreverRegistroBin(arquivoBin, &registros[i]);
        }
        
        //Inserção no proxRRN
        else {

            //Inserção do novo registro
            fseek(arquivoBin, 17 + cabecalho.proxRRN * 80, SEEK_SET);
            escreverRegistroBin(arquivoBin, &registros[i]);
            cabecalho.proxRRN++;        
        }
    }

    //Arquivo será fechado: consistente
    cabecalho.status = '1';

    fseek(arquivoBin, 0, SEEK_SET);
    escreverCabecalhoBin(arquivoBin, &cabecalho);

    fclose(arquivoBin);
    //free(registros);

    BinarioNaTela(nomeArquivoBin);
}

// Funcionalidade [5] - Update
void update(char* nomeArquivoBin, int nAtualizacoes) {

    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");

    char status;
    fread(&status, sizeof(char), 1, arquivoBin);

    // Verificando consistência do arquivo
    if(status == '0') { // inconsistente
        printf("Falha no processamento do arquivo.\n");
        fclose(arquivoBin);
        return;
    }

    //Arquivo aberto: inconsistente
    status = '0';
    fseek(arquivoBin, 0, SEEK_SET);
    fwrite(&status, sizeof(char), 1, arquivoBin);
    

    Busca *busca = (Busca*) malloc(2*nAtualizacoes*sizeof(Busca));
    recebeFiltros(busca, 2*nAtualizacoes);
    
    //Todo busca[i].filtro[j], com j par, representa os valores de atualização da busca i
    //Todo busca[i].filtro[j], com j ímpar, representa os valores de filtro da busca i
    
    for (int i = 0; i < 2*nAtualizacoes; i += 2) {

        fseek(arquivoBin, 17, SEEK_SET); // Pular cabeçalho
        
        Registro registro;

        int offsetAtual = 17;
        while (lerRegistroBin(arquivoBin, &registro) != -1) {
            
            if (registro.removido == '1') continue; // Pular registros removidos

            offsetAtual += 80;
            if (compararRegistroComFiltros(&registro, &busca[i])) {
                utils_atualizarRegistroComFiltros(busca[i+1], arquivoBin, offsetAtual);
            }
        }

        free(busca[i].filtro); // Liberar memória dos filtros da busca atual
    }

    //Arquivo será fechado: status consistente
    status = '1';
    fseek(arquivoBin, 0, SEEK_SET);
    fwrite(&status, sizeof(char), 1, arquivoBin);

    fclose(arquivoBin);

    BinarioNaTela(nomeArquivoBin);
}