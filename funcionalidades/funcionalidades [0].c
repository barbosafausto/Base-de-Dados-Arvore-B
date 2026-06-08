#include "funcionalidades.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Funcionalidade [1] - Create Table
// [1] Implementação da funcionalidade de criação da tabela binária a partir do CSV
void createTable(char *nomeArquivoCSV, char *nomeArquivoBin) {

    
    // --- Abrir arquivos
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


    // --- Inicializar o cabeçalho como inconsistente
    // cabecalho->status = '0': status será definido como '0' na função initCabecalho 
    Cabecalho cabecalho;
    registro_initCabecalho(&cabecalho);
    
    // Cabeçalho será escrito em arquivo
    registro_escreverCabecalhoBin(arquivoBin, &cabecalho);

    
    // --- Processamento das linhas do CSV
    char linha[200];
    
    // Registros (linhas)
    Registro registro;

    // Lista para armazenar estações já vistas
    NodeNome *listaNomes = NULL; 
    
    // Lista para armazenar pares de estações já vistos
    NodePares *listaPares = NULL; 

    
    // Pular a linha de cabeçalho do CSV
    fgets(linha, sizeof(linha), arquivoCSV); 

    // --- Loop até o final do arquivo CSV (NULL)
    while(fgets(linha, sizeof(linha), arquivoCSV) != NULL) {
        
        // Linha vazia (precaução)
        if(linha[0] == '\n' || linha[0] == '\r' || linha[0] == '\0') continue; 

        // Remover o caractere de nova linha
        linha[strcspn(linha, "\r\n")] = '\0'; 

        // Processamento dos campos via strsep
        registro_processaCSV(&registro, linha);

        // Escrever o registro no arquivo binário
        registro_escreverRegistroBin(arquivoBin, &registro);

        // Contagem de Estações
        if(registro.tamNomeEstacao > 0) {
            if(utils_adicionarEstacaoUnica(&listaNomes, registro.nomeEstacao)) 
                cabecalho.nroEstacoes++;
        }

        // Contagem de Pares
        if(registro.codProxEstacao != -1) {
            if(utils_adicionarParUnico(&listaPares, registro.codEstacao, registro.codProxEstacao))
                cabecalho.nroParesEstacao++;
        }
        
        // Incrementar o próximo RRN disponível
        cabecalho.proxRRN++; 
    }

    //Atualizamos a consistência para '1' e escrevemos o cabeçalho
    registro_gerenciaCabecalho(&cabecalho, arquivoBin, 1, 0);

    // Fechamento dos arquivos
    fclose(arquivoCSV);
    fclose(arquivoBin);
    
    // Liberar memória das listas (contagem de estações e pares)
    utils_liberarUtils(listaNomes, listaPares); 

    BinarioNaTela(nomeArquivoBin);
}

// --- Funcionalidade [2] - Select
// [2] Implementação da funcionalidade que seleciona os registros do arquivo
void selectFromTable(char *nomeArquivoBin) {

    // Abertura do Binário
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb");
    if (arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // --- Verificação de consistência
    Cabecalho cabecalho; 
    fread(&cabecalho.status, sizeof(char), 1, arquivoBin);
    if (!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 1))
        return;
    
    // --- Pular cabeçalho
    fseek(arquivoBin, 17, SEEK_SET); 

    // --- Leitura e impressão dos registros
    Registro registro;
    int statusLeitura;
    int existeRegistro = 0;

    // Loop Leitura
    while ((statusLeitura = registro_lerRegistroBin(arquivoBin, &registro)) != -1) {
        if (statusLeitura == 1) {
            registro_imprimirRegistro(&registro);
            existeRegistro = 1;
        }
    }

    // Caso de não-existência do registro
    if (!existeRegistro) {
        printf("Registro inexistente.\n");
    }

    // Fechamento do arquivo
    fclose(arquivoBin);
}

// Funcionalidade [3] - Where
// [3] Implementação da funcionalidade Select com Filtros // Atualizada!
void selectWhere(FILE *arquivoBin, Busca *busca) {

    //O arquivo de dados fornecido já está aberto.

    // Não faremos verificação de consistência do arquivo, 
    // pois já fizemos na função selecWhereAB, que chamou esta.

    // Pular cabeçalho
    fseek(arquivoBin, 17, SEEK_SET); 
        
    Registro registro;
    int encontrouRegistro = 0;

    // Leitura até EOF
    while (registro_lerRegistroBin(arquivoBin, &registro) != -1) {
        
        // Pular registros removidos
        // A função faz a checagem; ela só lê o registro inteiro se ele não estiver removido.
        if (registro.removido == '1') continue; 

        if (utils_compararRegistroComFiltros(&registro, busca)) {
            registro_imprimirRegistro(&registro);
            encontrouRegistro = 1;
        }
    }

    if (!encontrouRegistro) 
        printf("Registro inexistente.\n");

    // Formatação da saída
    printf("\n");
    
    // Liberar memória dos filtros da busca atual
    free(busca->campo); 
}


// Funcionalidade [4] - Delete
// [4] Implementação da funcionalidade de deletar registros que atendem a filtros
void deleteWhere(char *nomeArquivoBin, int nRemocoes) {

    // Abertura do arquivo
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // --- Setando consistência do arquivo para '0'
    // Leitura do cabeçalho
    Cabecalho cabecalho;
    registro_lerCabecalho(&cabecalho, arquivoBin);
    
    // Verificando consistência do arquivo
    if (!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 0))
        return;
    
    for (int i = 0; i < nRemocoes; i++) {

        // Recebimento dos campos que compõem os filtros de todas as remoções
        Busca busca;
        utils_recebeCampos(&busca);
        
        // Processamento das Remoções
            
        // Pular cabeçalho
        fseek(arquivoBin, 17, SEEK_SET); 
        
        Registro registro;
        int offsetAtual = 17;

        while (registro_lerRegistroBin(arquivoBin, &registro) != -1) {
            
            offsetAtual += 80;

            // Pular registros removidos
            if (registro.removido == '1') continue; 

            if (utils_compararRegistroComFiltros(&registro, &busca)) 
                registro_deletarRegistro(&registro, &cabecalho, arquivoBin, offsetAtual);
        }
        
        // Liberar memória dos filtros da busca atual
        free(busca.campo);
    }

    // Recontagem do número de estações e pares.
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoBin, 1);
    
    // --- Uma vez que as deleções finalizaram, podemos escrever o novo cabeçalho
    // O arquivo será fechado como consistente: status = 1
    registro_gerenciaCabecalho(&cabecalho, arquivoBin, 1, 0);
    
    // Fechamento do arquivo
    fclose(arquivoBin);

    BinarioNaTela(nomeArquivoBin);
}

// Funcionalidade [5] - Insert
// [5] Implementação da funcionalidade que insere registros usando remoção lógica
void insertInto(char *nomeArquivoBin, int nInsercoes) {

    // Abertura do arquivo
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // Cabeçalho será setado para inconsistente
    // Leitura do cabeçalho
    Cabecalho cabecalho;
    registro_lerCabecalho(&cabecalho, arquivoBin);
    
    // Verificando consistência do arquivo
    if(!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 0))
        return;
    
    // Nesse caso, os campos compõem um registro
    for (int i = 0; i < nInsercoes; i++) {

        //Leitura do registro
        Registro registro;
        registro_lerRegistro(&registro);

        //Inserção no topo da pilha
        if (cabecalho.topo != -1) {

            //Atualização do topo da pilha
            fseek(arquivoBin, TAM_CABECALHO + cabecalho.topo * TAM_REGISTRO + 1, SEEK_SET);
            fread(&cabecalho.topo, sizeof(int), 1, arquivoBin);

            //Inserção do novo registro
            fseek(arquivoBin, - sizeof(int) - 1, SEEK_CUR);
            registro_escreverRegistroBin(arquivoBin, &registro);
        }
        
        //Inserção no proxRRN
        else {

            //Inserção do novo registro
            fseek(arquivoBin, TAM_CABECALHO + cabecalho.proxRRN * TAM_REGISTRO, SEEK_SET);
            registro_escreverRegistroBin(arquivoBin, &registro);
            cabecalho.proxRRN++;        
        }
    }

    // Atualização dos Pares de Estação
    utils_contaNroEstacoesNroPares(&cabecalho, arquivoBin, 0);

    // Escrevendo cabecalho e setando como consistente 
    registro_gerenciaCabecalho(&cabecalho, arquivoBin, 1, 0);

    // Fechamento do arquivo
    fclose(arquivoBin);

    BinarioNaTela(nomeArquivoBin);
}

// Funcionalidade [6] - Update
// [6] Implementação da funcionalidade que atualiza registros
void update(char* nomeArquivoBin, int nAtualizacoes) {

    // Abertura do arquivo
    FILE *arquivoBin = fopen(nomeArquivoBin, "rb+");
    if(arquivoBin == NULL) {
        printf("Falha no processamento do arquivo.\n");
        return;
    }

    // Gerenciamento do cabeçalho
    Cabecalho cabecalho;
    fread(&cabecalho.status, sizeof(char), 1, arquivoBin);
    
    // Verificando consistência do arquivo
    if(!registro_gerenciaCabecalho(&cabecalho, arquivoBin, 0, 0))
        return;

    for (int i = 0; i < 2*nAtualizacoes; i += 2) {

        //Recebimento dos filtros e valores
        Busca filtros, valores;
        utils_recebeCampos(&filtros);
        utils_recebeCampos(&valores);
        
        
        // Pular cabeçalho
        fseek(arquivoBin, 17, SEEK_SET); 
        
        // Registros que serão lidos e reescritos
        Registro registro;

        int offsetAtual = 17;
        while (registro_lerRegistroBin(arquivoBin, &registro) != -1) {
            
            offsetAtual += 80;
            
            // Pular registros removidos
            if (registro.removido == '1') continue; 

            if (utils_compararRegistroComFiltros(&registro, &filtros)) {
                utils_atualizarRegistroComFiltros(valores, arquivoBin, offsetAtual);
            }
        }
        
        // Liberar memória do update atual
        free(filtros.campo); // filtros
        free(valores.campo); // atualizações
    }

    // ---- Arquivo será fechado: status consistente
    // Não vamos usar a função "registro_gerenciaCabecalho" aqui, porque ela escreve o cabecalho inteiro
    // Como só alteramos o status, usar a função com a opção escreveConsistente = 1 seria overkill
    cabecalho.status = '1';
    fseek(arquivoBin, 0, SEEK_SET);
    fwrite(&cabecalho.status, sizeof(char), 1, arquivoBin);

    // Fechamento do Arquivo
    fclose(arquivoBin);

    BinarioNaTela(nomeArquivoBin);
}
