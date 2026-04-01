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
    while(cur != NULL) {
        // Nessa condição, não importa a ordem (A->B == B->A)
        if((cur->cod1 == cod1 && cur->cod2 == cod2) || (cur->cod1 == cod2 && cur->cod2 == cod1) || (cod1 == -1 || cod2 == -1)) {
            return 0; // Par já existe ou é inválido
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