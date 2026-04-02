#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "registro.h"

// Usando listas encadeadas para armazenar estações e pares de estações já vistos,
// para evitar que os valores estejam duplicados.

// Dessa forma, mesmo que o arquivo CSV seja muito grande, a memória usada para armazenar
// os nomes e pares de estações será relativamente pequena.

int adicionarEstacaoUnica(NodeNome **head, char *nome) {

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

int adicionarParUnico(NodePares **head, int cod1, int cod2) {

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

void liberarUtils(NodeNome *headNomes, NodePares *headPares) {
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

void utils_contaNroEstacoesNroPares(Cabecalho *cabecalho, FILE *arquivoBin) {

    if (cabecalho == NULL || arquivoBin == NULL) return;

    //Volta cursor para o início, pós cabeçalho
    fseek(arquivoBin, 17, SEEK_SET);

    int nroEstacoes = 0;
    int nroParesEstacao = 0;

    //Enquanto não alcançar o fim do arquivo
    Registro *registro = (Registro*) malloc(sizeof(Registro));
    NodeNome *n_head = NULL;
    NodePares *p_head = NULL;
    while(lerRegistroBin(arquivoBin, registro) != -1) {

        if (registro->removido == '1') continue;

        if (adicionarEstacaoUnica(&n_head, registro->nomeEstacao))
            nroEstacoes++;
    
        if (adicionarParUnico(&p_head, registro->codEstacao, registro->codProxEstacao))
            nroParesEstacao++;
    }

    cabecalho->nroEstacoes = nroEstacoes;
    cabecalho->nroParesEstacao = nroParesEstacao;

    free(registro);
    liberarUtils(n_head, p_head);
}

int compararRegistroComFiltros(Registro *registro, Busca *busca) {
    for (int i = 0; i < busca->mFiltros; i++) {
        Filtro *filtro = &busca->filtro[i];
        int atendeCriterio = 0;

        // Campos inteiros
        if (filtro->isString == 0) {
            int valorRegistro;

            if      (strcmp(filtro->nomeCampo, "codEstacao") == 0)      valorRegistro = registro->codEstacao;
            else if (strcmp(filtro->nomeCampo, "codLinha") == 0)        valorRegistro = registro->codLinha;
            else if (strcmp(filtro->nomeCampo, "codProxEstacao") == 0)  valorRegistro = registro->codProxEstacao;
            else if (strcmp(filtro->nomeCampo, "distProxEstacao") == 0) valorRegistro = registro->distProxEstacao;
            else if (strcmp(filtro->nomeCampo, "codLinhaIntegra") == 0) valorRegistro = registro->codLinhaIntegra;
            else if (strcmp(filtro->nomeCampo, "codEstIntegra") == 0)   valorRegistro = registro->codEstIntegra;

            if (valorRegistro == filtro->valorInt) {
                atendeCriterio = 1;
            }
        }
        // Campos string 
        else if (filtro->isString == 1) {
            char *valorRegistro;
            int tamValorRegistro;

            if (strcmp(filtro->nomeCampo, "nomeEstacao") == 0) {
                valorRegistro = registro->nomeEstacao;
                tamValorRegistro = registro->tamNomeEstacao;
            } else if (strcmp(filtro->nomeCampo, "nomeLinha") == 0) {
                valorRegistro = registro->nomeLinha;
                tamValorRegistro = registro->tamNomeLinha;
            }

            // Se filtro for NULO -> tamanho no registro = 0
            if (strcmp(filtro->valorString, "NULO") == 0) {
                if (tamValorRegistro == 0) { // Campo é NULO
                    atendeCriterio = 1;
                }
            } 
            // Comparação normal de strings
            else {
                if (strcmp(valorRegistro, filtro->valorString) == 0) {
                    atendeCriterio = 1;
                }
            }
        }
        
        if (!atendeCriterio) return 0; // Se não atender a nenhum dos filtros

    }

    return 1; // Passou nos filtros
}


void recebeFiltros(Busca *busca, int nBuscas) {

    for (int i = 0; i < nBuscas; i++) {
        scanf("%d", &busca[i].mFiltros);
        busca[i].filtro = (Filtro *) malloc(busca[i].mFiltros * sizeof(Filtro));

        for (int j = 0; j < busca[i].mFiltros; j++) {
            scanf("%s", busca[i].filtro[j].nomeCampo);

            if (strcmp(busca[i].filtro[j].nomeCampo, "nomeEstacao") == 0 ||
                strcmp(busca[i].filtro[j].nomeCampo, "nomeLinha") == 0) {
                // 
                    // Campo string
                    ScanQuoteString(busca[i].filtro[j].valorString);
                busca[i].filtro[j].isString = 1;

            } 
            
            else {
                // Campo int ou NULO
                char valor[64];
                scanf(" %s", valor);
                if (strcmp(valor, "NULO") == 0) {
                    busca[i].filtro[j].valorInt = -1; // Valor padrão para NULO
                } else {
                    busca[i].filtro[j].valorInt = atoi(valor);
                }
                busca[i].filtro[j].isString = 0;
            }
        }
    }
}

void utils_atualizarRegistroComFiltros(Busca busca, FILE *arquivoBin, int offsetAtual) {

    int offsetRegistro = offsetAtual - 80;

    Registro registro;
    fseek(arquivoBin, offsetRegistro, SEEK_SET);
    lerRegistroBin(arquivoBin, &registro);

    for (int j = 0; j < busca.mFiltros; j++) {

        //Agora, vamos atualizar o registro
        if      (strcmp(busca.filtro[j].nomeCampo, "codEstacao") == 0)      registro.codEstacao = busca.filtro[j].valorInt;
        else if (strcmp(busca.filtro[j].nomeCampo, "codLinha") == 0)        registro.codLinha = busca.filtro[j].valorInt;
        else if (strcmp(busca.filtro[j].nomeCampo, "codProxEstacao") == 0)  registro.codProxEstacao = busca.filtro[j].valorInt;
        else if (strcmp(busca.filtro[j].nomeCampo, "distProxEstacao") == 0) registro.distProxEstacao = busca.filtro[j].valorInt; 
        else if (strcmp(busca.filtro[j].nomeCampo, "codLinhaIntegra") == 0) registro.codLinhaIntegra = busca.filtro[j].valorInt;
        else if (strcmp(busca.filtro[j].nomeCampo, "codEstIntegra") == 0)   registro.codEstIntegra = busca.filtro[j].valorInt;
        
        else if      (strcmp(busca.filtro[j].nomeCampo, "nomeLinha") == 0) {

            strcpy(registro.nomeEstacao, busca.filtro[j].valorString);           
            registro.tamNomeEstacao = strlen(registro.nomeEstacao);
        }

        else if (strcmp(busca.filtro[j].nomeCampo, "codLinha") == 0) {
            
            strcpy(registro.nomeLinha, busca.filtro[j].valorString);           
            registro.tamNomeLinha = strlen(registro.nomeLinha);

        }
    }

    //Agora é só escrever o registro :D
    fseek(arquivoBin, offsetRegistro, SEEK_SET);
    escreverRegistroBin(arquivoBin, &registro);

    //Restaurando a posição para não corromper o loop
    fseek(arquivoBin, offsetAtual, SEEK_SET);
}