#include <ucontext.h>



typedef struct co{
	ucontext_t ucont;
	
}co_t;

int mycoroutines_init(co_t *main);

int mycoroutines_create(co_t *co, void (body)(void *),void *arg);

int mycoroutines_switchto(co_t *co);

int mycoroutines_destroy(co_t *co);
