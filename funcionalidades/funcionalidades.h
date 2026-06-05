#ifndef FUNCIONALIDADES_H

    #define FUNCIONALIDADES_H

    #include "../registro/registro.h"
    #include "../utils/utils.h"
    #include "../fornecidas/fornecidas.h"
    #include "../arvoreb/arvoreb.h"

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <stdbool.h>

    void createTable(char *nomeArquivoCSV, char *nomeArquivoBin);
    void selectFromTable(char *nomeArquivoBin);
    // A função selectWhere é reutilizada pela função selectWhereAB
    // Isso ocorre quando os filtros não possuem codEstacao.
    // Por isso, mudamos os parâmetros da selectWhere para adequar a nova situação. :D
    void selectWhere(FILE *arquivoBin, Busca *busca);
    void deleteWhere(char *nomeArquivoBin, int nRemocoes);
    
    void insertInto(char *nomeArquivoBin, int nInsercoes);
    void update(char *nomeArquivoBin, int nAtualizacoes);



    void createIndex(char* nomeArquivoDadosBin, char* nomeArquivoIndiceBin);
    void selectWhereAB(char* nomeArquivoDadosBin, char* nomeArquivoIndiceBin, int nBuscas);

#endif