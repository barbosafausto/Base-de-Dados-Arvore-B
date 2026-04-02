#include "registro.h"

void initCabecalho(Cabecalho *cabecalho) {
    cabecalho->status = '0';            // Inconsistente
    cabecalho->topo = -1;               // Nenhum registro removido (Pilha vazia)
    cabecalho->proxRRN = 0;             // Próximo RRN disponível é o primeiro
    cabecalho->nroEstacoes = 0;         // Nenhuma estação armazenada
    cabecalho->nroParesEstacao = 0;     // Nenhum par de estações armazenado
}

void escreverCabecalhoBin(FILE *arquivo, Cabecalho *cabecalho) {
    // Escrever o cabeçalho no arquivo binário (17 bytes)
    fwrite(&cabecalho->status,          1, 1, arquivo);
    fwrite(&cabecalho->topo,            4, 1, arquivo);
    fwrite(&cabecalho->proxRRN,         4, 1, arquivo);
    fwrite(&cabecalho->nroEstacoes,     4, 1, arquivo);
    fwrite(&cabecalho->nroParesEstacao, 4, 1, arquivo);
    // Como o cabeçalho vai ser exatamente 17 bytes e com a estrutura definida,
    // não utilizei sizeof() para determinar tamanho da escrita (afinal, já está determinado)
}

int lerRegistroBin(FILE *arquivo, Registro *registro) {
    // Fim do arquivo
    if(fread(&registro->removido, sizeof(char), 1, arquivo) != 1) 
        return -1;
    
    // Se estiver removido, não precisa ler o restante do registro
    if(registro->removido == '1') {
        fseek(arquivo, 79, SEEK_CUR);
        return 0;
    }

    // Ler campos fixos do registro
    fread(&registro->proximo,         sizeof(int), 1, arquivo);
    fread(&registro->codEstacao,      sizeof(int), 1, arquivo);
    fread(&registro->codLinha,        sizeof(int), 1, arquivo);
    fread(&registro->codProxEstacao,  sizeof(int), 1, arquivo);
    fread(&registro->distProxEstacao, sizeof(int), 1, arquivo);
    fread(&registro->codLinhaIntegra, sizeof(int), 1, arquivo);
    fread(&registro->codEstIntegra,   sizeof(int), 1, arquivo);

    // Ler os campos variáveis do registro
    fread(&registro->tamNomeEstacao, sizeof(int), 1, arquivo);
    if(registro->tamNomeEstacao > 0) {
        fread(registro->nomeEstacao, sizeof(char), registro->tamNomeEstacao, arquivo);
        registro->nomeEstacao[registro->tamNomeEstacao] = '\0'; // Fim da string
    } else {
        registro->nomeEstacao[0] = '\0'; // Nome vazio
    }

    fread(&registro->tamNomeLinha, sizeof(int), 1, arquivo);
    if(registro->tamNomeLinha > 0) {
        fread(registro->nomeLinha, sizeof(char), registro->tamNomeLinha, arquivo);
        registro->nomeLinha[registro->tamNomeLinha] = '\0'; // Fim da string
    } else {
        registro->nomeLinha[0] = '\0'; // Nome vazio
    }

    // Pular lixo
    int bytesLidos = 37 + registro->tamNomeEstacao + registro->tamNomeLinha;
    if(bytesLidos < TAM_REGISTRO) {
        fseek(arquivo, TAM_REGISTRO - bytesLidos, SEEK_CUR);
    }

    return 1; // Sucesso
}

void escreverRegistroBin(FILE *arquivo, Registro *registro) {
    // Escrever campos fixos do registro
    fwrite(&registro->removido,        sizeof(char), 1, arquivo);
    fwrite(&registro->proximo,         sizeof(int), 1, arquivo);
    fwrite(&registro->codEstacao,      sizeof(int), 1, arquivo);
    fwrite(&registro->codLinha,        sizeof(int), 1, arquivo);
    fwrite(&registro->codProxEstacao,  sizeof(int), 1, arquivo);
    fwrite(&registro->distProxEstacao, sizeof(int), 1, arquivo);
    fwrite(&registro->codLinhaIntegra, sizeof(int), 1, arquivo);
    fwrite(&registro->codEstIntegra,   sizeof(int), 1, arquivo);

    // Escrever campos variáveis do registro
    fwrite(&registro->tamNomeEstacao, sizeof(int), 1, arquivo);
    if(registro->tamNomeEstacao > 0) {
        fwrite(registro->nomeEstacao, sizeof(char), registro->tamNomeEstacao, arquivo);
    }

    fwrite(&registro->tamNomeLinha, sizeof(int), 1, arquivo);
    if(registro->tamNomeLinha > 0) {
        fwrite(registro->nomeLinha, sizeof(char), registro->tamNomeLinha, arquivo);
    }

    // Preencher o restante do registro com lixo
    int bytesEscritos = 37 + registro->tamNomeEstacao + registro->tamNomeLinha;
    while(bytesEscritos < TAM_REGISTRO) {
        // fwrite(&lixo, sizeof(char), 1, arquivo);
        fputc('$', arquivo);
        bytesEscritos++;
    }
}

void imprimirRegistro(Registro *registro) {
    // Nunca serão NULO
    printf("%d ", registro->codEstacao);
    printf("%s ", registro->nomeEstacao);

    // Podem ser NULO
    (registro->codLinha == -1)          ? printf("NULO ")  : printf("%d ",  registro->codLinha);
    (registro->tamNomeLinha == 0)       ? printf("NULO ")  : printf("%s ",  registro->nomeLinha);
    (registro->codProxEstacao == -1)    ? printf("NULO ")  : printf("%d ",  registro->codProxEstacao);
    (registro->distProxEstacao == -1)   ? printf("NULO ")  : printf("%d ",  registro->distProxEstacao);
    (registro->codLinhaIntegra == -1)   ? printf("NULO ")  : printf("%d ",  registro->codLinhaIntegra);
    (registro->codEstIntegra == -1)     ? printf("NULO\n") : printf("%d\n", registro->codEstIntegra);

    return;
}

void deletarRegistro(Registro *registro, Cabecalho *cabecalho, FILE *arquivoBin, int offsetAtual) {

    int byteOffsetRegistro = offsetAtual - 80;

    // Atualização dos dados do registro em memória
    registro->removido = '1';                 
    registro->proximo = cabecalho->topo;      // O "próximo" aponta para o antigo topo

    // Posicionamento do cursor
    fseek(arquivoBin, byteOffsetRegistro, SEEK_SET);

    // Update de informações do registro
    fwrite(&registro->removido, sizeof(char), 1, arquivoBin);
    fwrite(&registro->proximo, sizeof(int), 1, arquivoBin);

    //Atualização do topo da pilha (no cabeçalho)
    //Será escrito em disco quando acabarem as deleções
    cabecalho->topo = byteOffsetRegistro / 80;

    //Volta para o offset correto do loop de deleções
    fseek(arquivoBin, offsetAtual, SEEK_SET);
}    

void lerCabecalho(Cabecalho *cabecalho, FILE *arquivoBin) {

    fread(&cabecalho->status,          1, 1, arquivoBin);
    fread(&cabecalho->topo,            4, 1, arquivoBin);
    fread(&cabecalho->proxRRN,         4, 1, arquivoBin);
    fread(&cabecalho->nroEstacoes,     4, 1, arquivoBin);
    fread(&cabecalho->nroParesEstacao, 4, 1, arquivoBin);
}

Registro *utils_leRegistros(int nRegistros) {

    Registro *registros = (Registro*) malloc(nRegistros * sizeof(Registro));
    for (int i = 0; i < nRegistros; i++) {

        // 100 bytes: comprimento seguro
        char valor[100];
        int valorInt; 

        registros[i].removido = '0';
        registros[i].proximo = -1;
        
        for (int j = 0; j < 8; j++) { 

            // String
            if (j == 1 || j == 3) 
                ScanQuoteString(valor);

            // Int
            else {
                scanf(" %s", valor);
                valorInt = (strcmp(valor, "NULO") == 0) ? -1 : atoi(valor);
            }
            
        //Inserção dos dados no registro
        if (j == 0)      registros[i].codEstacao = valorInt;
        else if (j == 1) {
            
            strcpy(registros[i].nomeEstacao, valor);
            registros[i].tamNomeEstacao = strlen(valor);
        }
        else if (j == 2) registros[i].codLinha = valorInt;
        else if (j == 3) {

            strcpy(registros[i].nomeLinha, valor);
            registros[i].tamNomeLinha = strlen(valor);
        }

        else if (j == 4) registros[i].codProxEstacao = valorInt;
        else if (j == 5) registros[i].distProxEstacao = valorInt;
        else if (j == 6) registros[i].codLinhaIntegra = valorInt;
        else             registros[i].codEstIntegra = valorInt;
        
        }
    }

    return registros;
}
