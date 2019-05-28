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



thread_node* list_add(thr_t *to_be_added) {
	thread_node *new_node,*curr;
	curr = head;
	new_node = (thread_node*)malloc(sizeof(thread_node));
	while (curr->next!=head)
		curr = curr->next;
	
	curr->next = new_node;
	new_node->next = head;
	new_node->thread = to_be_added;
	
	thread_count++;
	
	return new_node;
	
}

void list_init() {
	
	head = (thread_node*)malloc(sizeof(thread_node));
	head->next = head;
	head->thread = NULL;
}

void list_remove() {
	
	
// 	if (to_be_deleted!=NULL)
//  		free=(to_be_deleted);
	thread_count--;
	thread_node *curr;
	curr = head;
	to_be_deleted = running;
	while(curr->next != running) {
		curr = curr->next;
	}
	
	curr->next = curr->next->next;
	
}
void scheduler() {
	if(thread_count>0){
		thread_node *temp;
	// 	temp = running->thread->context;
		temp = running;
		if (running->next == head) 
			running = running->next->next;
		else {
			running = running->next;
		}
		
		swapcontext(&temp->thread->context, &running->thread->context);
	}
	
}

int mythreads_yield(){
	sigprocmask(SIG_BLOCK, &a, &b);
	thread_node *temp;
	temp = running;
	if (running->next == head) 
		running = running->next->next;
	else {
		running = running->next;
	}
	sigprocmask(SIG_SETMASK, &b, NULL);
	swapcontext(&temp->thread->context, &running->thread->context);

	return 0;
}

int mythreads_join(thr_t *thr) {
	
	
	while(!thr->finished){
		
		
		
		mythreads_yield();
		}
	return 0;	
}
void kill_thread() {
	sigprocmask(SIG_BLOCK, &a, &b); 
	//bgazei apo ti lista
// 	printf(" 	IAMDONE\n");
 	running->thread->finished = 1;
// 	printf("Running1: %d\n",running);
 	list_remove();
	
	sigprocmask(SIG_SETMASK, &b, NULL);
	
	
 	mythreads_yield();
	

}
int mythreads_init() {
	thr_t main_thread;
	getcontext(&main_thread.context);
	list_init();
	to_be_deleted = NULL;
	sigemptyset(&a);
	sigaddset(&a, SIGPROF); 
	
	
	getcontext(&finish_context);
	finish_context.uc_link=0;
	finish_context.uc_stack.ss_sp=malloc(MEM);
	finish_context.uc_stack.ss_size=MEM;
	finish_context.uc_stack.ss_flags=0;
	makecontext(&finish_context,(void*)kill_thread,0);
	
// 	main_thread.context.uc_link=0;
//  	main_thread.context.uc_stack.ss_sp = malloc(MEM);
// 	main_thread.finished = 0;
// 	main_thread.context.uc_stack.ss_size=MEM;
// 	main_thread.context.uc_stack.ss_flags=0;
// 	makecontext(&main_thread.context,NULL,0);
	
	running = list_add(&main_thread);
	

	
	thread_count = 1;
	//Sighandling
	struct itimerval it;
	struct sigaction act, oact;

	act.sa_handler = scheduler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;

	sigaction(SIGPROF, &act, &oact); 
	// Start itimer
	it.it_interval.tv_sec = 0;
	it.it_interval.tv_usec = 500000;
	it.it_value.tv_sec = 0;
	it.it_value.tv_usec = 500000;
	setitimer(ITIMER_PROF, &it, NULL);
	
	return 0;
}


int mythreads_destroy(thr_t *thr) {

	free(thr->context.uc_stack.ss_sp);
	kill_thread();
	return 0;
	
}



int mythreads_create(thr_t *thr, void (body)(void *),void *arg){
	sigprocmask(SIG_BLOCK, &a, &b); 
	
	thr->finished = 0;

	getcontext(&thr->context);
	thr->context.uc_sigmask=b;
	thr->context.uc_link=&finish_context;
	thr->context.uc_stack.ss_sp=malloc(MEM);
	thr->context.uc_stack.ss_size=MEM;
	thr->context.uc_stack.ss_flags=0;
	makecontext(&thr->context, (void*)body, 1, arg);
	
	
	list_add(thr);
	
	
	sigprocmask(SIG_SETMASK, &b, NULL);
	
	return 0;
	
}

thr_t* dequeue(sem_t *s) {
	
	thread_node *to_be_removed;
	
	to_be_removed = s->fifo_head;
	s->fifo_head = s->fifo_head->next;
	

// 	free(to_be_removed);
	
	if(s->fifo_head == NULL) 
		s->fifo_tail = NULL;
	
	return(to_be_removed->thread);
}



int enqueue(sem_t *s){
	
	thread_node *new_node;
	new_node = malloc(sizeof(thread_node));
	new_node->thread = running->thread;
	new_node->next = NULL;
	if (!s->blocked) {
		s->fifo_tail = new_node;
		s->fifo_head = new_node;
	}
	else {
		
		s->fifo_tail->next = new_node;
		s->fifo_tail = new_node;
	}
	return 0;
}

int mythreads_sem_init(sem_t *s, int val){
	sigprocmask(SIG_BLOCK, &a, &b); 
	
	s->val = val;
	s->blocked = 0;
	
	s->fifo_head = NULL;
	s->fifo_tail = NULL;
	
	sigprocmask(SIG_SETMASK, &b, NULL);

	return 0;
}

int mythreads_sem_down(sem_t *s) {
	sigprocmask(SIG_BLOCK, &a, &b); 
	if (s->val >= 1) {
		s->val--;
		sigprocmask(SIG_SETMASK, &b, NULL);
	}
	else {
		enqueue(s);
		s->blocked++;
		
		list_remove();
		if (running->next == head) 
			running = running->next->next;
		
		else 
			running = running->next;
		
		sigprocmask(SIG_SETMASK, &b, NULL);
		while(thread_count==0) {}
  		swapcontext(&s->fifo_tail->thread->context,&running->thread->context);
// 		raise(SIGPROF);
	}
	
	sigprocmask(SIG_SETMASK, &b, NULL);
	
	return 0;
	
	

}


int mythreads_sem_up(sem_t *s){
	sigprocmask(SIG_BLOCK, &a, &b); 
	if(s->val == 0){
		if(s->blocked!=0){
			thr_t *unblocked;
			unblocked = dequeue(s);
			s->blocked--;
			list_add(unblocked);
			sigprocmask(SIG_SETMASK, &b, NULL);
			return 0;
		}
		
	}
	s->val++;
	sigprocmask(SIG_SETMASK, &b, NULL);
	return 0;
}



int mythreads_sem_destroy(sem_t *s){
	sigprocmask(SIG_BLOCK, &a, &b); 

	if(s->fifo_head==NULL){
		
	}
	else{
		thread_node* curr=s->fifo_head,*temp;
		while(curr!=s->fifo_tail){
			temp = curr;
			curr = curr->next;
			free(temp);
		}
		
	}
	sigprocmask(SIG_SETMASK, &b, NULL);
	return 0;
}
