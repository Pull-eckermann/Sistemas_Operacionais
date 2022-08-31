// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include <signal.h>     // biblioteca para manipulação de sinais 
#include <sys/time.h>
#include <string.h>


// Estrutura que define um Task Control Block (TCB)
typedef struct task_t{
  struct task_t *prev, *next, *suspensas;		// ponteiros para usar em filas
  int id ;			                  // identificador da tarefa
  unsigned int processor_time;             // Conta o tempo de processamento da tarefa
  unsigned int execution_time;             // Conta o tempo de  execução total
  unsigned int activations;                // Conta o número de ativações da tarefa
  short status ;			            // pronta, rodando, suspensa, ...
  int static_prio, dinamic_prio;  // Prioridades estática e dinâmica de execução
  short quantum;                  // Valor do quantum de duração de cada tarefa
  short preemptable ;			        // pode ser preemptada?
  int despertar;                  // instante em que a tarefa suspensa deverá acordar
  int exit_code;
  ucontext_t context ;			      // contexto armazenado da tarefa
} task_t ;

// estrutura que define um semáforo
typedef struct{
  int t_count;    //Contador de tarefas
  struct task_t *down_t;   //Fila do semáforo que guarda as tarefas suspensas
  int destruido;
} semaphore_t ;

// estrutura que define uma fila de mensagens
typedef struct buffer{
  struct buffer *prev, *next;
  void *product;
} buffer;

typedef struct{
  buffer *msg_q;
  int tam;
  semaphore_t s_buffer;
  semaphore_t s_item;
  semaphore_t s_vaga;
} mqueue_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;


#endif

