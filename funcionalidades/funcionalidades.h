#ifndef FUNCIONALIDADES_H

    #define FUNCIONALIDADES_H

    #include "../registro/registro.h"
    #include "../utils/utils.h"
    #include "../fornecidas/fornecidas.h"

    void createTable(char *nomeArquivoCSV, char *nomeArquivoBin);
    void selectFromTable(char *nomeArquivoBin);
    void selectWhere(char *nomeArquivoBin, int nBuscas);
    void deleteWhere(char *nomeArquivoBin, int nRemocoes);
    void insertInto(char *nomeArquivoBin, int nInsercoes);
    void update(char *nomeArquivoBin, int nAtualizacoes);

#endif