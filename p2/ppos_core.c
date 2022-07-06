//ERICK ECKERMANN CARDOSO GRR20186075
#include <stdlib.h>
#include <ucontext.h>
#include "ppos_data.h"
#include "ppos.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

ucontext_t current_context, main_context;

// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init (){
  setvbuf (stdout, 0, _IONBF, 0);
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg){
  getcontext(&task->context);
  makecontext(&task->context, (void*) (*start_func), 1, arg);

}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit (int exit_code){
  
}

// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
  swapcontext(&current_context, &task->context);
} 

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id (){

}