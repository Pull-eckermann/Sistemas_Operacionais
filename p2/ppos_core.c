//ERICK ECKERMANN CARDOSO GRR20186075
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include "ppos_data.h"
#include "ppos.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

int cont_id = 0;
task_t main_cont, *current;

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init (){
  getcontext(&main_cont.context);         //Salva o contexto da tarefa main
  main_cont.id = 0;
  main_cont.next = NULL;
  main_cont.prev = NULL;  
  current = &main_cont;                   //Tarefa corrente aponta para main
  setvbuf (stdout, 0, _IONBF, 0);         //desativa o buffer da saida padrao (stdout), usado pela função printf
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg){
  char *stack ;
  task->next = NULL;
  task->prev = NULL;
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
  current = task;
  makecontext(&task->context, (void*) (*start_func), 1, arg);

  //Soma 1 no contador do ID e atribui esse id à tarefa criada
  cont_id ++;
  task->id = cont_id;
  //Tarefa corrente volta a ser a main
  current = &main_cont;

  return cont_id;
}

// Termina a tarefa corrente voltando a main, indicando um valor de status encerramento
void task_exit (int exit_code){
  swapcontext(&current->context, &main_cont.context);
  printf("%d: fim\n",exit_code);
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
  task_t *atual;
  atual = current;
  current = task;
  swapcontext(&atual->context, &task->context);

  return 0; 
} 

// retorna o identificador da tarefa corrente (main_cont deve ser 0)
int task_id (){
  return current->id;
}