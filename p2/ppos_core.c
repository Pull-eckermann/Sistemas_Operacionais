//ERICK ECKERMANN CARDOSO GRR20186075
#include <stdlib.h>
#include <ucontext.h>
#include "ppos_data.h"
#include "ppos.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

int cont_id = 0;
task_t *main, *current;

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init (){
  getcontext(&main->context);
  main->id = 0;
  //setvbuf (stdout, 0, _IONBF, 0);
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg){
  char *stack ;
  getcontext(&task->context);
  stack = malloc (STACKSIZE) ;
  //Cria a pilha da tarefa
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
  makecontext(&task->context, (void*) (*start_func), 1, arg);
  //Soma 1 no contador do ID e atribui esse id à tarefa criada
  cont_id ++;
  task->id = cont_id;
  return cont_id;
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exit_code){
  swapcontext(&current->context, &main->context);
  printf("%d: fim\n");
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
  swapcontext(&current->context, &task->context);
} 

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id (){
  
}