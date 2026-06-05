#include <stdio.h>
#include "funcionalidades/funcionalidades.h"

/* DUPLA
15512767 - José Fausto Vital Barbosa
16876293 - João Pedro Boaretto
*/

int main() {

    int funcionalidade, nBuscas;
    char arquivo1[64], arquivo2[64];

    if (scanf("%d", &funcionalidade) != 1) {
        return 0;
    }

    switch (funcionalidade) {
        case 1:
            scanf(" %s %s", arquivo1, arquivo2);
            createTable(arquivo1, arquivo2);
            break;

        case 2:
            scanf(" %s", arquivo1);
            selectFromTable(arquivo1);
            break;

        case 3:
            scanf(" %s %d", arquivo1, &nBuscas);

            // Por causa da função selectWhereAB, a função selectWhere não precisa mais ser chamada aqui.
            // selectWhere(arquivo1, nBuscas);

            // A função selectWhereAB será responsável por chamá-la caso não haja o campo "codEstacao"
            // entre os filtros de entrada :D
        
            // Por isso, chamaremos ela.
            scanf(" %s %s %d", arquivo1, arquivo2, &nBuscas);
            selectWhereAB(arquivo1, arquivo2, nBuscas);
            break;

        case 4:
            scanf(" %s %d", arquivo1, &nBuscas);
            deleteWhere(arquivo1, nBuscas);
            break;

        case 5:
            scanf(" %s %d", arquivo1, &nBuscas);
            insertInto(arquivo1, nBuscas);
            break;

        case 6:
            scanf(" %s %d", arquivo1, &nBuscas);
            update(arquivo1, nBuscas);
            break;

        case 7:
            scanf(" %s %s", arquivo1, arquivo2);
            createIndex(arquivo1, arquivo2);
            break;

        case 8:
            scanf(" %s %s %d", arquivo1, arquivo2, &nBuscas);
            selectWhereAB(arquivo1, arquivo2, nBuscas);
            break;
    }

    return 0;
}