#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Usando listas encadeadas para armazenar estações e pares de estações já vistos,
// para evitar que os valores estejam duplicados.

// Dessa forma, mesmo que o arquivo CSV seja muito grande, a memória usada para armazenar
// os nomes e pares de estações será relativamente pequena.
int utils_adicionarEstacaoUnica(NodeNome **head, char *nome) {

    if(nome == NULL || nome[0] == '\0' || strcmp(nome, "NULO") == 0) return 0;
    
    // Verifica se já existe
    NodeNome *cur = *head;
    while(cur != NULL) {
        if(strcmp(cur->nome, nome) == 0) {
            return 0; // Estação já existe
        }
        cur = cur->prox;
    }

    // Se não existe
    NodeNome *novo = (NodeNome *) malloc(sizeof(NodeNome));
    if(novo == NULL) {
        printf("Erro de alocação de memória para nome de estação.");
        return 0;
    }

    novo->nome = strdup(nome);
    novo->prox = *head; // Insere no início (mais fácil, já tem o head e não precisa de ordem)
    *head = novo;

    return 1; // Estação não existe
}


int utils_adicionarParUnico(NodePares **head, int cod1, int cod2) {

    // Verifica se já existe
    NodePares *cur = *head;
    
    // Par inválido
    if (cod1 == -1 || cod2 == -1) return 0;

    while(cur != NULL) {
        // Nessa condição, não importa a ordem (A->B == B->A)
        if((cur->cod1 == cod1 && cur->cod2 == cod2) || (cur->cod1 == cod2 && cur->cod2 == cod1)) {
            return 0; // Par já existe 
        }
        cur = cur->prox;
    }

    // Se não existe
    NodePares *novo = (NodePares *) malloc(sizeof(NodePares));
    if(novo == NULL) {
        printf("Erro de alocação de memória para par de estações.");
        return 0;
    }

    novo->cod1 = cod1;
    novo->cod2 = cod2;
    novo->prox = *head; // Insere no início
    *head = novo;

    return 1; // Par não existe
}


void utils_liberarUtils(NodeNome *headNomes, NodePares *headPares) {
    // Libera lista de nomes
    NodeNome *curNomes = headNomes;
    while(curNomes != NULL) {
        NodeNome *temp = curNomes;
        curNomes = curNomes->prox;
        free(temp->nome); // Libera a string alocada
        free(temp); // Libera o nó
    }

    // Libera lista de pares
    NodePares *curPares = headPares;
    while(curPares != NULL) {
        NodePares *temp = curPares;
        curPares = curPares->prox;
        free(temp); // Libera o nó
    }
}


void utils_contaNroEstacoesNroPares(Cabecalho *cabecalho, FILE *arquivoBin, int Delete) {

    //Volta cursor para o início, pós cabeçalho
    fseek(arquivoBin, 17, SEEK_SET);

    int nroEstacoes = 0;
    int nroParesEstacao = 0;

    //Enquanto não alcançar o fim do arquivo
    Registro *registro = (Registro*) malloc(sizeof(Registro));
    NodeNome *n_head = NULL;
    NodePares *p_head = NULL;
    while(registro_lerRegistroBin(arquivoBin, registro) != -1) {

        if (registro->removido == '1') continue;

        // Atualização dos pares
        if (utils_adicionarParUnico(&p_head, registro->codEstacao, registro->codProxEstacao))
            nroParesEstacao++;
        
        // Se não for a funcionalidade "deleteWhere", não precisamos atualizar as estações, apenas os pares.
        if (!Delete) continue;

        // Atualização das estações (funcionalidade insertInto)
        if (utils_adicionarEstacaoUnica(&n_head, registro->nomeEstacao))
            nroEstacoes++;
    }

    cabecalho->nroParesEstacao = nroParesEstacao;
    if (Delete)
        cabecalho->nroEstacoes = nroEstacoes;


    free(registro);
    utils_liberarUtils(n_head, p_head);
}


void utils_recebeCampos(Busca *busca, int nBuscas) {

    for (int i = 0; i < nBuscas; i++) {

        // Número de campos
        scanf("%d", &busca[i].mCampos);
        busca[i].campo = (Campo*) malloc(busca[i].mCampos * sizeof(Campo));

        // Leitura dos campos
        for (int j = 0; j < busca[i].mCampos; j++) {
            scanf("%s", busca[i].campo[j].nomeCampo);

            // Campo string
            if (strcmp(busca[i].campo[j].nomeCampo, "nomeEstacao") == 0 ||
                strcmp(busca[i].campo[j].nomeCampo, "nomeLinha") == 0) {
                 
                    ScanQuoteString(busca[i].campo[j].valorString);
                    busca[i].campo[j].isString = 1;

            } 
            
            // Campo int ou NULO
            else {
                
                char valor[64];
                scanf(" %s", valor);

                busca[i].campo[j].valorInt = (strcmp(valor, "NULO") == 0) ? -1 : atoi(valor);
                busca[i].campo[j].isString = 0;
            }
        }
    }
}


int utils_compararRegistroComFiltros(Registro *registro, Busca *busca) {

    for (int i = 0; i < busca->mCampos; i++) {

        Campo *campo = &busca->campo[i];
        int atendeCriterio = 0;

        // Campos inteiros
        if (campo->isString == 0) {
            int valorRegistro;

            if      (strcmp(campo->nomeCampo, "codEstacao") == 0)      valorRegistro = registro->codEstacao;
            else if (strcmp(campo->nomeCampo, "codLinha") == 0)        valorRegistro = registro->codLinha;
            else if (strcmp(campo->nomeCampo, "codProxEstacao") == 0)  valorRegistro = registro->codProxEstacao;
            else if (strcmp(campo->nomeCampo, "distProxEstacao") == 0) valorRegistro = registro->distProxEstacao;
            else if (strcmp(campo->nomeCampo, "codLinhaIntegra") == 0) valorRegistro = registro->codLinhaIntegra;
            else if (strcmp(campo->nomeCampo, "codEstIntegra") == 0)   valorRegistro = registro->codEstIntegra;

            if (valorRegistro == campo->valorInt) {
                atendeCriterio = 1;
            }
        }
        
        // Campos string 
        else if (campo->isString == 1) {
            char *valorRegistro;
            int tamValorRegistro;

            if (strcmp(campo->nomeCampo, "nomeEstacao") == 0) {

                valorRegistro = registro->nomeEstacao;
                tamValorRegistro = registro->tamNomeEstacao;
            } 
            
            else if (strcmp(campo->nomeCampo, "nomeLinha") == 0) {

                valorRegistro = registro->nomeLinha;
                tamValorRegistro = registro->tamNomeLinha;
            }

            // Se filtro for NULO -> tamanho no registro = 0
            if (strcmp(campo->valorString, "NULO") == 0) {
                if (tamValorRegistro == 0) { // Campo é NULO
                    atendeCriterio = 1;
                }
            } 
            // Comparação normal de strings
            else {
                if (strcmp(valorRegistro, campo->valorString) == 0) {
                    atendeCriterio = 1;
                }
            }
        }
        
        if (!atendeCriterio) return 0; // Se não atender a nenhum dos filtros

    }

    return 1; // Passou nos filtros
}


void utils_atualizarRegistroComFiltros(Busca busca, FILE *arquivoBin, int offsetAtual) {

    int offsetRegistro = offsetAtual - 80;

    Registro registro;
    fseek(arquivoBin, offsetRegistro, SEEK_SET);
    registro_lerRegistroBin(arquivoBin, &registro);

    for (int j = 0; j < busca.mCampos; j++) {

        //Agora, vamos atualizar o registro
        if      (strcmp(busca.campo[j].nomeCampo, "codEstacao") == 0)      registro.codEstacao = busca.campo[j].valorInt;
        else if (strcmp(busca.campo[j].nomeCampo, "codLinha") == 0)        registro.codLinha = busca.campo[j].valorInt;
        else if (strcmp(busca.campo[j].nomeCampo, "codProxEstacao") == 0)  registro.codProxEstacao = busca.campo[j].valorInt;
        else if (strcmp(busca.campo[j].nomeCampo, "distProxEstacao") == 0) registro.distProxEstacao = busca.campo[j].valorInt; 
        else if (strcmp(busca.campo[j].nomeCampo, "codLinhaIntegra") == 0) registro.codLinhaIntegra = busca.campo[j].valorInt;
        else if (strcmp(busca.campo[j].nomeCampo, "codEstIntegra") == 0)   registro.codEstIntegra = busca.campo[j].valorInt;
        
        else if      (strcmp(busca.campo[j].nomeCampo, "nomeLinha") == 0) {

            strcpy(registro.nomeEstacao, busca.campo[j].valorString);           
            registro.tamNomeEstacao = strlen(registro.nomeEstacao);
        }

        else if (strcmp(busca.campo[j].nomeCampo, "codLinha") == 0) {
            
            strcpy(registro.nomeLinha, busca.campo[j].valorString);           
            registro.tamNomeLinha = strlen(registro.nomeLinha);

        }
    }

    //Agora é só escrever o registro :D
    fseek(arquivoBin, offsetRegistro, SEEK_SET);
    registro_escreverRegistroBin(arquivoBin, &registro);

    //Não precisa dar fseek porque a última escrita posicionou a leitora na posição correta
    //fseek(arquivoBin, offsetAtual, SEEK_SET);
}