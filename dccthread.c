/**
 * @file dccthread.c
 * @author
 * @authors Vinícius Braga Freire (vinicius.braga@dcc.ufmg.br), Júnio Veras de
 * Jesus Lima (junio.veras@dcc.ufmg.br)
 * @brief
 * @version 0.1
 * @date 2023-04-03
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "dccthread.h"

typedef void (*callback_t)(int);

/**
 * @brief 
 * 
 */
struct dccthread {
    char* t_name;
    ucontext_t t_context;
    ucontext_t* parent_context;
    callback_t callback_f;
    int param;
};



/**
 * @brief 
 * 
 * @param func 
 * @param param 
 */
void dccthread_init(void (*func)(int), int param) 
{

}

/**
 * @brief Creates a dcc thread
 * 
 * @param name 
 * @param func 
 * @param param 
 * @return dccthread_t* 
 */
dccthread_t* dccthread_create(const char* name, void (*func)(int), int param) {
    dccthread_t* new_thread = (dccthread_t*) malloc(sizeof(dccthread_t) );
    // Instantiate the thread
    new_thread->t_name = name;
    new_thread->callback_f = func;
    new_thread->param = param;
    getcontext(new_thread->parent_context);
    char* stack = (char*)malloc(THREAD_STACK_SIZE * sizeof(char));
}