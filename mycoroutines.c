#include <ucontext.h>
#include <stdlib.h>
#define MEM 64000


typedef struct co{
	ucontext_t ucont;
}co_t;

co_t *running;

int mycoroutines_init(co_t *main){
	running = main;
	return getcontext(&main->ucont);	
}

int mycoroutines_create(co_t *co, void (body)(void *),void *arg){
	int res;
	res = getcontext(&co->ucont);
	co->ucont.uc_link=0;
	co->ucont.uc_stack.ss_sp=malloc(MEM);
	co->ucont.uc_stack.ss_size=MEM;
	co->ucont.uc_stack.ss_flags=0;
	makecontext(&co->ucont, (void*)body, 1, arg);
	return res;
}

int mycoroutines_switchto(co_t *co){
	co_t *temp;
	temp = running;
	running = co;

	return swapcontext(&temp->ucont, &co->ucont);
}

int mycoroutines_destroy(co_t *co){
	free(co->ucont.uc_stack.ss_sp);
	return 0;
	
}
