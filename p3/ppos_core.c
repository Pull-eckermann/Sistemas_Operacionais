//ERICK ECKERMANN CARDOSO GRR20186075
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include "ppos_data.h"
#include "ppos.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

int cont_id = 0;
int user_tasks = 0;
task_t main_cont;         //Tarefa Main
task_t despachante;       //Tarefa do despatcher 
task_t *prev = NULL;      //Ponteiro que aponta para a tarefa anterior à corrente
task_t *current = NULL;   //Ponteiro que aponta para a tarefa corrente
task_t *tarefas = NULL;   //Ponteiro para a lista de tarefas

//Schaduler com política FCFS implementada
task_t * schaduler(){
  task_t *aux = tarefas;
  tarefas->status = 3; //Status = 3 "EXECUTANDO"
  tarefas = tarefas->next;
  return aux;
}

void dispatcher (){
  task_t *proxima = NULL;
  task_t *main = prev;

  while(user_tasks > 0){
    proxima = schaduler();
    if(proxima != NULL){
      task_switch(proxima);
      if(proxima->status == 1){   //Status "TERMINADA"
          queue_remove((queue_t**) &tarefas, (queue_t*) proxima);
          free(proxima->context.uc_stack.ss_sp);
      }
    }
  }
  prev = main;
  task_exit(0);
}

// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield (){
  prev = current;
  current->status = 2; //Status = 2 "PRONTA"
  task_switch(&despachante);
}

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init (){
  getcontext(&main_cont.context);         //Salva o contexto da tarefa main
  main_cont.id = 0;
  main_cont.next = NULL;
  main_cont.prev = NULL;  
  current = &main_cont;                   //Tarefa corrente aponta para main
  setvbuf (stdout, 0, _IONBF, 0);         //desativa o buffer da saida padrao (stdout), usado pela função printf

  task_create (&despachante, dispatcher, 0);
  queue_remove ((queue_t**) &tarefas, (queue_t*) &despachante);
  user_tasks --;
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg){
  if (task == NULL){
    fprintf(stderr,"ERRO: ponteiro para tarefa a ser criada é nulo");
    return 2;
  }
  char *stack ;
  queue_append((queue_t**) &tarefas, (queue_t*) task);     //Adiciona a tarefa à lista de tarefas
  getcontext(&task->context);

  //Cria a pilha da tarefa
  stack = malloc (STACKSIZE) ;
  if (stack){
    task->context.uc_stack.ss_sp = stack ;
    task->context.uc_stack.ss_size = STACKSIZE ;
    task->context.uc_stack.ss_flags = 0 ;
    task->context.uc_link = 0 ;
  }
  else{
    perror ("Erro na criação da pilha: ") ;
    exit (1) ;
  }
  //Seta parâmetros do contexto
  user_tasks ++;
  current = task;
  makecontext(&task->context, (void*) (*start_func), 1, arg);
  //Soma 1 no contador do ID e atribui esse id à tarefa criada
  cont_id ++;
  task->id = cont_id;
  task->status = 2; // Status = 2 "PRONTA"
  //Tarefa corrente volta a ser a main
  current = &main_cont;

  return cont_id;
}

// Termina a tarefa corrente voltando a tarefa anterior, indicando um valor de status encerramento
void task_exit (int exit_code){
  user_tasks --;
  current->status = 1;  //status 1 = TERMINADA
  task_switch(prev);
  printf("%d: fim\n",exit_code);
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
  if(task == NULL){
    fprintf(stderr,"ERRO: ponteiro para tarefa é nulo");
    return 3;
  }
  prev = current;
  current = task;
  swapcontext(&prev->context, &task->context);

  return 0; 
} 

// retorna o identificador da tarefa corrente (main_cont deve ser 0)
int task_id (){
  return current->id;
}
