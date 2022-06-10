//GRR20186075 ERICK ECKERMANN CARDOSO  

#include <stdio.h>
#include "queue.h"

int queue_size (queue_t *queue){
    int cont = 0;

    for(queue_t *x = queue; x != NULL; x = x->next)
        cont ++;

    return cont;
}