//ERICK ECKERMANN CARDOSO GRR20186075
#include <stdlib.h>
#include <stdio.h>
#include "ppos_data.h"
#include "ppos.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

int cont_id = 0;
int user_tasks = 0;
task_t main_cont;         //Tarefa Main
task_t despachante;       //Tarefa do despatcher 
task_t *current = NULL;   //Ponteiro que aponta para a tarefa corrente
task_t *tarefas = NULL;   //Ponteiro para a lista de tarefas

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer;

//------------------------------------------------------------------------------
//Schaduler com política por prioridades dinâmicas implementada
task_t * schaduler(){
  task_t *aux;
  task_t *stressed = tarefas;
  //Acha a tarefa com maior prioridade dinãmica
  for(aux = tarefas->next; aux != tarefas; aux = aux->next)
    if (aux->dinamic_prio < stressed->dinamic_prio)
      stressed = aux;

  aux = tarefas;
  do{ //Diminui prioridades dinâmincas em 1 em todas as tarefas
    aux->dinamic_prio --;
    aux = aux->next;
  }while(aux != tarefas);
  
  stressed->dinamic_prio = stressed->static_prio;
  stressed->status = 3; //Status = 3 "EXECUTANDO"
  tarefas = stressed;
  return stressed;
}
//------------------------------------------------------------------------------
// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio){
  if((prio < -20) || (prio > 20)){
    fprintf(stderr,"Erro: Prioridade deve estar entre -20 e +20\n");
    exit(10);
  }
  if(task == NULL){
    current->static_prio = prio;
    current->dinamic_prio = prio;
  }else{
    task->static_prio = prio;
    task->dinamic_prio = prio;
  }
}
//------------------------------------------------------------------------------
// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task){
  if(task == NULL)
    return current->static_prio;
  return task->static_prio;
}
//------------------------------------------------------------------------------
void dispatcher (){
  task_t *proxima = NULL;

  while(user_tasks > 0){
    proxima = schaduler();    //Acha a próxima tafera a ser executada
    if(proxima != NULL){
      proxima->quantum = 20;
      task_switch(proxima);
      if(proxima->status == 1){   //Status "TERMINADA"
          queue_remove((queue_t**) &tarefas, (queue_t*) proxima);   //Remove da fila de prontas
          free(proxima->context.uc_stack.ss_sp);                    //Da free na tarefa
      }
    }
  }
  task_exit(0);
}
//------------------------------------------------------------------------------
// libera o processador para a próxima tarefa, retornando à fila de tarefas
// prontas ("ready queue")
void task_yield (){
  current->status = 2; //Status = 2 "PRONTA"
  task_switch(&despachante);
}
//------------------------------------------------------------------------------
// tratador do sinal do timer
void tratador_ticks (int signum){
  if(current->preemptable == 1){ // verifica se a tarefa pode ser preemptada 
    current->quantum --;
    if(current->quantum <= 0)
      task_yield();              //Se o quantum chegar a 0 volta para o dispatcher
  }
}
//------------------------------------------------------------------------------
// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init (){
  getcontext(&main_cont.context);         //Salva o contexto da tarefa main
  main_cont.id = 0;
  main_cont.next = NULL;
  main_cont.prev = NULL;
  main_cont.preemptable = 1;  
  current = &main_cont;                   //Tarefa corrente aponta para main
  setvbuf (stdout, 0, _IONBF, 0);         //desativa o buffer da saida padrao (stdout), usado pela função printf

  //Cria a tarefa dispatcher e remove ela da fila de tarefas
  task_create (&despachante, dispatcher, 0);
  queue_remove ((queue_t**) &tarefas, (queue_t*) &despachante);
  user_tasks --;

  // registra a ação para o sinal de timer SIGALRM
  action.sa_handler = tratador_ticks ;
  sigemptyset (&action.sa_mask) ;
  action.sa_flags = 0 ;
  if (sigaction (SIGALRM, &action, 0) < 0)
  {
    perror ("Erro em sigaction: ") ;
    exit (1) ;
  }
  // ajusta valores do temporizador
  timer.it_value.tv_usec = 1000;   // primeiro disparo, em micro-segundos
  timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
  timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
  timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos
  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if (setitimer (ITIMER_REAL, &timer, 0) < 0)
  {
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }
}
//------------------------------------------------------------------------------
// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg){
  if (task == NULL){
    fprintf(stderr,"ERRO: ponteiro para tarefa a ser criada é nulo");
    return 2;
  }
  user_tasks ++;
  current = task;
  task_setprio (task, 0);
  queue_append((queue_t**) &tarefas, (queue_t*) task);     //Adiciona a tarefa à lista de tarefas
  getcontext(&task->context);

  //Cria a pilha da tarefa
  char *stack ;
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
  makecontext(&task->context, (void*) (*start_func), 1, arg);
  //Seta parâmetros da tarefa
  cont_id ++;
  task->id = cont_id;
  task->status = 2; // Status = 2 "PRONTA"
  task->dinamic_prio = task->static_prio;
  if (task->id != 1)
    task->preemptable = 1;

  //Tarefa corrente volta a ser a main
  current = &main_cont;
  return cont_id;
}
//------------------------------------------------------------------------------
// Termina a tarefa corrente voltando a tarefa anterior, indicando um valor de status encerramento
void task_exit (int exit_code){
  user_tasks --;
  current->status = 1;  //status 1 = TERMINADA
  if(current->id > 1) 
    task_switch(&despachante);
  else
    task_switch(&main_cont);
  printf("%d: fim\n",exit_code);
}
//------------------------------------------------------------------------------
// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
  task_t *prev; 
  if(task == NULL){
    fprintf(stderr,"ERRO: ponteiro para tarefa é nulo");
    return 3;
  }
  prev = current;
  current = task;
  swapcontext(&prev->context, &task->context);

  return 0; 
} 
//------------------------------------------------------------------------------
// retorna o identificador da tarefa corrente (main_cont deve ser 0)
int task_id (){
  return current->id;
}
