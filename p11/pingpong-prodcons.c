// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Teste de semáforos (light)

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "queue.h"

#define VAGAS 5

task_t p1, p2, p3, c1, c2;
semaphore_t s_buffer, s_item, s_vaga;

typedef struct buffer{
   struct buffer *prev, *next;
   int product;
} buffer;

buffer *sistema;

// corpo da thread produtor
void produtor (void * arg){
   while (1){
      task_sleep (1000);   
      int rn = rand() %100;   //Cria item aleatório ente 0 e 99
      buffer *item = malloc(sizeof(buffer)); //Malloca pra adicionar na fila

      sem_down (&s_vaga);
      sem_down (&s_buffer);

      item->product = rn;
      item->prev = NULL;   //Seta como nulo pra inserção na fila
      item->next = NULL;
      queue_append((queue_t**) &sistema, (queue_t*) item);
      printf("%s produziu %d\n", (char *)arg, item->product);

      sem_up (&s_buffer);
      sem_up (&s_item);
   }
}

// corpo da thread consumidor
void consumidor (void * arg){
   buffer *item;
   while (1){
      sem_down (&s_item);     
      sem_down (&s_buffer);
      
      if(queue_size((queue_t*) sistema) > 0){
         item = sistema;   //Recebe sistema pra não dar segmentation fault
         queue_remove((queue_t**) &sistema, (queue_t*) sistema);
         printf("%s consumiu %d\n", (char *)arg, item->product);
         free(item); 
      }  

      sem_up (&s_buffer);
      sem_up (&s_vaga);

      task_sleep (1000);
   }
}

int main (int argc, char *argv[])
{
   printf ("main: inicio\n") ;

   ppos_init () ;

   // cria semaforos
   sem_create (&s_buffer, 1);
   sem_create (&s_item, 0);
   sem_create (&s_vaga, VAGAS);

   // cria tarefas
   task_create (&p1, produtor, "p1") ;
   task_create (&p2, produtor, "p2") ;
   task_create (&p3, produtor, "p3");
   task_create (&c1, consumidor, "                             c1") ;
   task_create (&c2, consumidor, "                             c2") ;

   // aguarda p1 encerrar
   task_join (&p1);

   sem_destroy(&s_buffer);
   sem_destroy(&s_item);
   sem_destroy(&s_vaga);

   printf ("main: fim\n") ;
   task_exit (0) ;

   exit (0) ;
}