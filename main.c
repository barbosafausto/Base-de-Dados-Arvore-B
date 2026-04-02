#include <stdio.h>
#include "funcionalidades.h"

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
            selectWhere(arquivo1, nBuscas);
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
    }

    return 0;
}