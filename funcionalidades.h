#ifndef FUNCIONALIDADES_H
#define FUNCIONALIDADES_H

void createTable(char *nomeArquivoCSV, char *nomeArquivoBin);
void selectFromTable(char *nomeArquivoBin);
void selectWhere(char *nomeArquivoBin, int nBuscas);

void deleteWhere(char *nomeArquivoBin, int nRemocoes);
void insertInto(char *nomeArquivoBin, int nInsercoes);
void update(char *nomeArquivoBin, int nAtualizacoes);

#endif