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
    //cabecalho.status = '0';   // Já é definido como '0' na função initCabecalho

    // Escrever o cabeçalho no arquivo binário (17 bytes)
    escreverCabecalhoBin(arquivoBin, &cabecalho);

    
    // Processamento das linhas do CSV
    char linha[1024];
    Registro registro;
    
    NodeNome *listaNomes = NULL; // Lista para armazenar estações já vistas
    NodePares *listaPares = NULL; // Lista para armazenar pares de estações já vistos
    
    
    fgets(linha, sizeof(linha), arquivoCSV); // Pular a linha de cabeçalho do CSV
    while(fgets(linha, sizeof(linha), arquivoCSV) != NULL) {

        // if(linha[0] == '\n' || linha[0] == '\r' || linha[0] == '\0') continue;
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
    cabecalho.status = '1'; // Tabela consistente

    // debbug
    printf("Status: %c, Topo: %d, ProxRRN: %d, nroEstacoes: %d, nroParesEstacao: %d\n", cabecalho.status, cabecalho.topo, cabecalho.proxRRN, cabecalho.nroEstacoes, cabecalho.nroParesEstacao);

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

    for (int i = 0; i < nBuscas; i++) {
        scanf("%d", &busca[i].mFiltros);
        busca[i].filtro = (Filtro *) malloc(busca[i].mFiltros * sizeof(Filtro));

        for (int j = 0; j < busca[i].mFiltros; j++) {
            scanf("%s", busca[i].filtro[j].nomeCampo);

            if (strcmp(busca[i].filtro[j].nomeCampo, "nomeEstacao") == 0 ||
                strcmp(busca[i].filtro[j].nomeCampo, "nomeLinha") == 0) {
                    // Campo string
                    ScanQuoteString(busca[i].filtro[j].valorString);
                    busca[i].filtro[j].isString = 1;
            } else {
                // Campo int ou NULO
                char valor[64];
                scanf("%s", valor);
                if (strcmp(valor, "NULO") == 0) {
                    busca[i].filtro[j].valorInt = -1; // Valor padrão para NULO
                } else {
                    busca[i].filtro[j].valorInt = atoi(valor);
                }
                busca[i].filtro[j].isString = 0;
            }
        }
    }

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
        }

        // Formatação da saída
        printf("\n");

        free(busca[i].filtro); // Liberar memória dos filtros da busca atual
    }

    free(busca); // Liberar memória da busca
    fclose(arquivoBin);
}