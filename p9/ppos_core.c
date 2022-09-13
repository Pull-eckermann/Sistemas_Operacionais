//ERICK ECKERMANN CARDOSO GRR20186075
#include <stdlib.h>
#include <stdio.h>
#include "ppos_data.h"
#include "ppos.h"
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */
#define QUANTUM 20
#define PRONTA 1
#define SUSPENSA 2
#define TERMINADA 3
#define EXECUTANDO 4

int cont_id = 0;
int user_tasks = 0;
short big_lock = 0;	//Trava preempções em funções de sistema
unsigned int mile_clock = 0; //Contador de milesegundos
task_t main_cont;         //Tarefa Main
task_t despachante;       //Tarefa do dispatcher 
task_t *current = NULL;   //Ponteiro que aponta para a tarefa corrente
task_t *tarefas = NULL;   //Ponteiro para a lista de tarefas PRONTAS
task_t *dormitorio = NULL;      //Ponteiro para a lista de tarefas adormecidas

struct sigaction action ;   // estrutura que define um tratador de sinal (deve ser global ou static)
struct itimerval timer;     // estrutura de inicialização to timer

//------------------------------------------------------------------------------
// informa as tarefas o valor corrente do mile_clock
unsigned int systime (){
  return mile_clock;
}
//-----------------------------------------------------------------------------
//Funções que administram a trava de preempção
void lock(){
  big_lock = 1;
}

void unlock(){
  big_lock = 0;
}
//------------------------------------------------------------------------------
// retorna o identificador da tarefa corrente (main_cont deve ser 0)
int task_id (){
  return current->id;
}
//------------------------------------------------------------------------------
// define a prioridade estática de uma tarefa (ou a tarefa atual)
void task_setprio (task_t *task, int prio){
  lock();
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
  unlock();
}
//------------------------------------------------------------------------------
// retorna a prioridade estática de uma tarefa (ou a tarefa atual)
int task_getprio (task_t *task){
  if(task == NULL)
    return current->static_prio;
  return task->static_prio;
}
//------------------------------------------------------------------------------
//Schaduler com política por prioridades dinâmicas implementada
task_t * schaduler(){
  lock();
  //Caso a fila estiver vazia retorna NULL
  if(tarefas == NULL)
    return NULL;

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
  stressed->status = EXECUTANDO;
  tarefas = stressed;
  unlock();
  return stressed;
}
//------------------------------------------------------------------------------
void dispatcher (){
  task_t *proxima = NULL;
  task_t *aux;

  while(user_tasks > 0){
    proxima = schaduler();    //Acha a próxima tarefa a ser executada
    
    do{   //Percorre a lista de tarefas dormindo e libera quem já passou o tempo  
      if(dormitorio != NULL){
        aux = dormitorio;
        do{
          if(aux->despertar <= systime()){
            aux = aux->next;
            task_resume(aux->prev, &dormitorio);
            break;
          }
          aux = aux->next;
        }while(aux!=dormitorio && dormitorio);
      }
    }while(aux!=dormitorio && dormitorio);

    if(proxima != NULL){      //Caso em que a fila não estava vazia
      proxima->quantum = QUANTUM;
      task_switch(proxima);
      if(proxima->status == TERMINADA){   //Status "TERMINADA"
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
  current->status = PRONTA; //Tarefa volta à fila de "PRONTA"
  task_switch(&despachante);
}
//------------------------------------------------------------------------------
// tratador do sinal do timer
void tratador_ticks (int signum){
  mile_clock++;                  // mile_clock incrementa a cada milesegundo
  current->processor_time++;
  if(current->preemptable == 1 && !big_lock){ // verifica se a tarefa pode ser preemptada 
    current->quantum --;
    if(current->quantum <= 0)
      task_yield();              //Se o quantum chegar a 0 volta para o dispatcher
  }
}
//------------------------------------------------------------------------------
// Inicializa o sistema operacional; deve ser chamada no inicio do main()
void ppos_init (){
  getcontext(&main_cont.context);         //Salva o contexto da tarefa main
  user_tasks ++;
  main_cont.status = PRONTA; // Status = "PRONTA"
  main_cont.id = 0;
  main_cont.next = NULL;
  main_cont.prev = NULL;
  queue_append((queue_t**) &tarefas, (queue_t*) &main_cont);     //Adiciona a tarefa à lista de tarefas
  main_cont.activations = 1;
  main_cont.processor_time = 0;
  main_cont.execution_time = systime();
  main_cont.preemptable = 1;
  main_cont.quantum = QUANTUM;
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
  if (sigaction (SIGALRM, &action, 0) < 0){
    perror ("Erro em sigaction: ") ;
    exit (1) ;
  }
  // ajusta valores do temporizador
  timer.it_value.tv_usec = 1000;   // primeiro disparo, em micro-segundos
  timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if (setitimer (ITIMER_REAL, &timer, 0) < 0){
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }
}
//------------------------------------------------------------------------------
// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create (task_t *task, void (*start_func)(void *), void *arg){
  lock();
  if (task == NULL){
    fprintf(stderr,"ERRO: ponteiro para tarefa a ser criada é nulo");
    return 2;
  }
  user_tasks ++;
  current = task;
  task_setprio (task, 0);
  queue_append((queue_t**) &tarefas, (queue_t*) task);     //Adiciona a tarefa à lista de tarefas
  getcontext(&task->context);

  //Seta parâmetros da tarefa
  cont_id ++;
  task->id = cont_id;
  task->status = PRONTA; // Tarefa pertence a fila de prontas "PRONTA"
  task->activations = 1;
  task->processor_time = 0;
  task->execution_time = systime();
  if (task->id != 1)
    task->preemptable = 1;  //Se não for o dispatcher indica que é preemtavel

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

  //Tarefa corrente volta a ser a main
  current = &main_cont;
  unlock();
  return cont_id;
}
//------------------------------------------------------------------------------
// Termina a tarefa corrente voltando a tarefa anterior, indicando um valor de status encerramento
void task_exit (int exit_code){
  lock();
  user_tasks --;
  current->status = TERMINADA;
  current->execution_time = mile_clock - current->execution_time;   //Calcula tempo de execução total

  //Acorda todas as tarefas aguardando o encerramento de current
  if(current->suspensas){
    task_t *aux = current->suspensas;
    do{
      aux = aux->next;
      task_resume(aux, &current->suspensas);
    }while(current->suspensas);   //Enquanto a fila não for vazia
  }
  
  printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n",current->id,
                                        current->execution_time, current->processor_time, current->activations);
  current->exit_code = exit_code;
  unlock();
  if(current->id != 1)   //Indica que a tarefa foi criada depois do dispatcher
    task_switch(&despachante);
  exit(exit_code);
}
//------------------------------------------------------------------------------
// alterna a execução para a tarefa indicada
int task_switch (task_t *task){
  lock();
  task_t *prev;
  if(task == NULL){
    fprintf(stderr,"ERRO: ponteiro para tarefa é nulo");
    return 3;
  }
  current->activations++;   //Incrementa numero de ativações da tarefa
  prev = current;
  current = task;
  unlock(); 
  swapcontext(&prev->context, &task->context);

  return 0; 
}
//------------------------------------------------------------------------------
// suspende a tarefa atual na fila "queue"
void task_suspend (task_t **queue){
  lock();
  if(queue){
    current->status = SUSPENSA;
    queue_remove((queue_t**) &tarefas, (queue_t*) current);
    queue_append((queue_t**) queue, (queue_t*) current);
  }
  unlock();  
  task_yield();
}
//------------------------------------------------------------------------------
// acorda a tarefa indicada, que está suspensa na fila indicada
void task_resume (task_t *task, task_t **queue){
  lock();
  if(queue && task){
    task->status = PRONTA;
    queue_remove((queue_t**) queue, (queue_t*) task);
    queue_append((queue_t**) &tarefas, (queue_t*) task);
  }
  unlock();
}
//------------------------------------------------------------------------------
// a tarefa corrente aguarda o encerramento de outra task
int task_join (task_t *task){
  lock();
  if ((!task) || (task->status == TERMINADA))   //Tarefa indicada não existe ou já foi terminada
    return -1;
  task_suspend(&task->suspensas); 
  unlock();
  return(task->exit_code);   //Retorna o codigo de encerramento da task...
}                     //convencionado como o task->id (Talvez mude??)
//------------------------------------------------------------------------------
// suspende a tarefa corrente por t milissegundos
void task_sleep (int t) {
  lock();
  queue_remove((queue_t**) &tarefas, (queue_t*) current);
  queue_append((queue_t**) &dormitorio, (queue_t*) current);
  current->despertar = systime() + t;    //Calcula o instante em que a tarefa devera acordar
  unlock();
  task_yield();
}