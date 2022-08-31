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
int a_lock = 0; //Trava para as operações atômicas
unsigned int mile_clock = 0; //Contador de milesegundos
task_t main_cont;         //Tarefa Main
task_t despachante;       //Tarefa do dispatcher 
task_t *current = NULL;   //Ponteiro que aponta para a tarefa corrente
task_t *tarefas = NULL;   //Ponteiro para a lista de tarefas PRONTAS
task_t *dormitorio = NULL;  //Ponteiro para a lista de tarefas ADORMECIDAS

struct sigaction action ;   // estrutura que define um tratador de sinal (deve ser global ou static)
struct itimerval timer;     // estrutura de inicialização to timer

//------------------------------------------------------------------------------
// informa as tarefas o valor corrente do mile_clock
unsigned int systime (){
  return mile_clock;
}
//-----------------------------------------------------------------------------
//Funções que administram a trava de preempção, impedindo troca de contexto
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
}
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
//------------------------------------------------------------------------------
//Funções para controle da região crítica dos semáforos
void enter_sc(int *lock){
  // atomic OR (Intel macro for GCC)
  while (__sync_fetch_and_or (lock, 1)) ;   // busy waiting
}
//------------------------------------------------------------------------------
void leave_sc(int *lock){
  (*lock) = 0 ;
}
//------------------------------------------------------------------------------
// cria um semáforo com valor inicial "value"
int sem_create (semaphore_t *s, int value){
  if (s){
    s->t_count = value;
    s->down_t = NULL;
    s->destruido = 0;
    return 0;
  }else
    return -1;
// A chamada retorna 0 em caso de sucesso ou -1 em caso de erro.
}
//------------------------------------------------------------------------------
// requisita o semáforo
// A chamada retorna 0 em sucesso ou -1 em erro(semáforo não existe ou foi destruído).
int sem_down(semaphore_t *s){
  if (s){
    enter_sc(&a_lock);  //Controla acesso à seção crítica
    s->t_count --;
    leave_sc(&a_lock);
    if (s->t_count < 0){
      //Se o contador for negativo suspende a tarefa
      task_suspend(&s->down_t);
      if(s->destruido)
        return -1;
    }
    return 0;
  }else
    return -1;
}
//------------------------------------------------------------------------------
// libera o semáforo
int sem_up(semaphore_t *s){
  if(s && !s->destruido) {
    enter_sc(&a_lock);  //Controla acesso à seção crítica
    s->t_count ++;
    leave_sc(&a_lock);
    if (s->t_count <= 0)
      //Acorda a primeira tarefa se tiver tarefas para acordar
      task_resume (s->down_t, &s->down_t); 
    return 0;
  }else
    return -1;
}
//------------------------------------------------------------------------------
// destroi o semáforo, liberando as tarefas bloqueadas
int sem_destroy(semaphore_t *s){
  if(s){
    enter_sc(&a_lock);  //Controla acesso à seção crítica
    s->destruido = 1;
    leave_sc(&a_lock);
    while(s->down_t != NULL)  //Acorda todas as tarefas
      task_resume(s->down_t, &s->down_t);
    return 0;
  }else
    return -1;
}
//------------------------------------------------------------------------------
// cria uma fila para até max mensagens de size bytes cada
int mqueue_create (mqueue_t *queue, int max, int size){
  //Fila de mensagens se inicia nula
  queue->msg_q = NULL;
  queue->tam = size;
  queue->destruido = 0;

  //Cria semáforos e confere se deu erro
  if(sem_create (&queue->s_buffer, 1) == -1)
    return -1;
  if (sem_create (&queue->s_item, 0) == -1)
    return -1;
  if(  sem_create (&queue->s_vaga, max) == -1)
    return -1;

  return 0;
}
//------------------------------------------------------------------------------
// envia uma mensagem para a fila
int mqueue_send (mqueue_t *queue, void *msg){
  lock();
  if(!queue->destruido && queue){
    //Confere se deu erro ao down ou se o semáforo foi destruído
    if(sem_down (&queue->s_vaga) == -1){
      unlock();
      return -1;
    }
    if(sem_down (&queue->s_buffer) == -1){
      unlock();
      return -1;
    }  
    //Aloca mensagem e adiciona na fila
    buffer *mensagem = malloc(sizeof(buffer));
    if (!mensagem){ //Erro no malloc
      sem_up (&queue->s_buffer);
      sem_up (&queue->s_item);
      unlock();
      return -1;
    }
    mensagem->prev = NULL;
    mensagem->next = NULL;
    bcopy(msg, &mensagem->product, queue->tam);
    queue_append((queue_t**) &queue->msg_q, (queue_t*) mensagem);

    //Libera semáforos
    sem_up (&queue->s_buffer);
    sem_up (&queue->s_item);
    unlock();
    return 0;
  }
  unlock();
  return -1;
}
//------------------------------------------------------------------------------
// recebe uma mensagem da fila
int mqueue_recv (mqueue_t *queue, void *msg){
  lock();
  if(!queue->destruido && queue){
    //Confere se deu erro ao down ou se o semáforo foi destruído 
    if(sem_down (&queue->s_item) == -1){
      unlock();
      return -1;
    }
    if(sem_down (&queue->s_buffer) == -1){
      unlock();	  
      return -1;
    }
    //Retira da fila a primeira mensagem disponível
    buffer *mensagem = queue->msg_q;
    queue_remove((queue_t**) &queue->msg_q, (queue_t*) queue->msg_q);
    bcopy(&mensagem->product, msg, queue->tam);
    free(mensagem);

    //Libera semáforos
    sem_up (&queue->s_buffer);
    sem_up (&queue->s_vaga);
    unlock();
    return 0;
  }
  unlock();
  return -1;
}
//------------------------------------------------------------------------------
// destroi a fila, liberando as tarefas bloqueadas
int mqueue_destroy (mqueue_t *queue){
  if(!queue)
    return -1;
  lock();
  //Libera mensagens na fila um por um
  buffer *destruir; 
  while(queue->msg_q){
    destruir = queue->msg_q;
    queue_remove((queue_t**) &queue->msg_q, (queue_t*) queue->msg_q);
    free(destruir);
  }
  free(queue->msg_q);
  queue->destruido = 1;
  //Destrói os semáforos
  sem_destroy(&queue->s_buffer);
  sem_destroy(&queue->s_item);
  sem_destroy(&queue->s_vaga);
  unlock();
  return 0;
}
//------------------------------------------------------------------------------
// informa o número de mensagens atualmente na fila
int mqueue_msgs (mqueue_t *queue){
  //Confere se não existe a fila indicada
  if(!queue)
    return -1;
  //Retorna o tamanho da fila
  lock();
  int tam = queue_size((queue_t *) queue->msg_q);
  unlock();
  return tam;
}
 
