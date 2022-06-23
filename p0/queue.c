//GRR20186075 ERICK ECKERMANN CARDOSO  

#include <stdio.h>
#include "queue.h"

//------------------------------------------------------------------------------
// Conta o numero de elementos na fila

int queue_size (queue_t *queue){
    int cont = 0;
    queue_t *aux = queue;

    if(queue != NULL)
        do{
            cont ++;
            aux = aux->next;
        }while( aux != queue);
    return cont;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo.

void queue_print (char *name, queue_t *queue, void print_elem (void*)){
    // Imprime "Saida Gerada"
    printf("%s: ",name);
    queue_t *andarilho = queue;

    // Se a fila estiver vazia imprime somente "[]"
    if (andarilho == NULL)
        printf("[]");
    else{
    //Caso contrário faz o processo de impressão normal
        printf("[");
        do{
            print_elem(andarilho);
            andarilho = andarilho->next;
            if(andarilho != queue)
                printf(" ");
        }while(andarilho != queue);
        printf("]");
    }
    printf("\n");
}
//------------------------------------------------------------------------------
// Insere um elemento no final da fila.

int queue_append (queue_t **queue, queue_t *elem){

    //Confere se a fila não existe e retorna menssagem de erro
    if(queue == NULL){
        fprintf(stderr,"A fila nao existe\n");
        return 1;
    }
    //Confere se o elem não existe e retorna menssagem de erro
    if(elem == NULL){
        fprintf(stderr,"O elemento nao existe\n");
        return 2;
    }
    //Confere se o elemente a ser incerido não pertence a outra fila
    if((elem->next != NULL) || (elem->prev != NULL)){
        fprintf(stderr,"O elemento ja pertence a uma fila\n");
        return 3;
    }

    //Faz a incersão no caso de uma fila vazia
    if(*queue == NULL){
        *queue = elem;
        elem->next = elem;
        elem->prev = elem;
        return 0;
    //Faz a incersão no caso de uma fila com 1 ou mais elementos, inserindo no final
    }else{
        queue_t *first = *queue;
        elem->next = first;
        elem->prev = first->prev;
        first->prev = elem;
        elem->prev->next = elem;
        return 0;
    }
}
//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.

int queue_remove (queue_t **queue, queue_t *elem){

    //Confere se a fila não existe e retorna menssagem de erro
    if(queue == NULL){
        fprintf(stderr,"A fila nao existe\n");
        return 1;
    }
    //Confere se a fila está vazia e retorna menssagem de erro caso esteja 
    if(*queue == NULL){
        fprintf(stderr,"A fila se encontra vazia\n");
        return 2;
    }
    //Confere se o elem não existe e retorna menssagem de erro
    if(elem == NULL){
        fprintf(stderr,"O elemento nao existe\n");
        return 3;
    }
    
    //Confere se o elemente a ser removido pertence à fila indicada
    queue_t *aux = *queue;
    do{
        //Se pertencer realiza o processo de remoção
        if(aux == elem){
            //Esse é o caso em que a fila possuí 1 elemento
            if(elem->next == elem || elem->prev == elem){
                elem->next = NULL;
                elem->prev = NULL;
                *queue = NULL;
                return 0;
            }
            //Esse é o caso em que a fila tem mais de um elemento
            else{
                elem->prev->next = elem->next;
                elem->next->prev = elem->prev;
                if(elem == *queue)
                    *queue = elem->next;
                elem->next = NULL;
                elem->prev = NULL;
                return 0;
            }
        }
        aux = aux->next;
    }while(aux != *queue);
    //Caso não pertença, cairá nesse caso e retorna erro
    fprintf(stderr,"O elemento nao pertence a fila, portanto nao pode ser removido\n");
    return 4;
}