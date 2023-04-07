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

/**
 * @brief The scheduler context, used to come back to the scheduler after an
 * thread execution
 *
 */
static ucontext_t main_ctx;
/**
 * @brief Threads list
 */
static struct dlist* threads_list;
/**
 * @brief An enumeration of all avaiable thread states.
 *
 */
enum u_int8_t { RUNNING, RUNNABLE, SEMI, FINISHED } THREAD_STATE;

typedef void (*callback_t)(int);

/**
 * @brief A struct that defines a DCC thread.
 *
 */
struct dccthread {
    const char* t_name;
    enum u_int8_t state;
    ucontext_t t_context;
};

/**
 * @brief
 *
 * @param func
 * @param param
 */
void dccthread_init(void (*func)(int), int param) {
    // Create the list to hold all the threads managed by the scheduler
    threads_list = dlist_create();

    // Change to the manager thread context and call the scheduler function
    // ucontext_t scheduler_ctx;
    if(getcontext(&main_ctx) == -1) {
        printf("Error while getting context\n");
        exit(EXIT_FAILURE);
    }
    dccthread_t* main_thread = dccthread_create("principal", func, param);

    // While there are threads to be computed
    while(threads_list->count) {
        struct dnode* cur = threads_list->head;
        // Iterate over thread lists
        while(cur) {
            dccthread_t* data = cur->data;
            if(data->state == RUNNABLE) {
                // Execute the thread function
                swapcontext(&main_ctx, &data->t_context);
                dlist_remove_from_node(threads_list, cur);
                break;
            }
            cur = cur->next;
        }
    }

    exit(EXIT_SUCCESS);
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
    dccthread_t* new_thread = (dccthread_t*)malloc(sizeof(dccthread_t));
    // Instantiate the thread
    new_thread->t_name = name;
    new_thread->state = RUNNABLE;
    // Create a new context and stack
    if(getcontext(&new_thread->t_context) == -1) {
        puts("Error while getting context...exiting\n");
        exit(EXIT_FAILURE);
    }
    char* stack = (char*)malloc(THREAD_STACK_SIZE * sizeof(char));
    new_thread->t_context.uc_link = &main_ctx;
    new_thread->t_context.uc_stack.ss_sp = stack;
    new_thread->t_context.uc_stack.ss_size = THREAD_STACK_SIZE;
    new_thread->t_context.uc_stack.ss_flags = 0;
    // Make sure that when the context is swapped the <func> is called with
    // <param> parametter
    makecontext(&new_thread->t_context, (void (*)())func, 1, param);
    // Add this thread to the end of the list of waiting threads
    dlist_push_right(threads_list, new_thread);
    //
    return new_thread;
}