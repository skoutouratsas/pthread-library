#include <ucontext.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/time.h>
#include "mycoroutines.h"
#define MEM 64000


typedef struct thr_t {
	ucontext_t context;
	int finished;
}thr_t;

typedef struct thread_node thread_node;

struct thread_node{
	thread_node *next;
	thr_t *thread;
};

typedef struct thread_node thread_node;

typedef struct sem_t{
	thread_node *fifo_head;
	thread_node *fifo_tail;
	int val;
	int blocked;
}sem_t;

thread_node *head;
thread_node *running,*to_be_deleted;
sigset_t a,b;
ucontext_t finish_context;

int thread_count;

thread_node* list_add(thr_t *to_be_added);

void list_init();

void list_remove();
void scheduler();

int mythreads_yield();

int mythreads_join(thr_t *thr);

void kill_thread();

int mythreads_init();

int mythreads_destroy(thr_t *thr);

int mythreads_create(thr_t *thr, void (body)(void *),void *arg);

thr_t* dequeue(sem_t *s) ;

int enqueue(sem_t *s);

int mythreads_sem_init(sem_t *s, int val);

int mythreads_sem_down(sem_t *s) ;

int mythreads_sem_up(sem_t *s);

int mythreads_sem_destroy(sem_t *s);
