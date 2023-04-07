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
 * @brief An enumeration of all avaiable thread states.
 *
 */
enum u_int8_t { RUNNING, RUNNABLE } THREAD_STATE;

/**
 * @brief A struct that defines a DCC thread.
 *
 */
struct dccthread {
    const char* t_name;
    enum u_int8_t state;
    ucontext_t t_context;
};

struct scheduler {
    /**
     * @brief The scheduler context, used to come back to the scheduler after an
     * thread execution
     *
     */
    ucontext_t main_ctx;
    /**
     * @brief Threads list menaged by the scheduler.
     */
    struct dlist* threads_list;
    /**
     * @brief The current thread being executed. When this pointer is NULL means
     * that the scheduler thread is running.
     *
     */
    dccthread_t* current_thread;
};

static scheduler_t scheduler;

typedef void (*callback_t)(int);

/**
 * @brief Function responsible for simulating a thread scheduler.
 *
 * @param func The function for the main thread to be spawned.
 * @param param Parameter to be passed to <func>
 */
void dccthread_init(void (*func)(int), int param) {
    // Create the list to hold all the threads managed by the scheduler
    scheduler.threads_list = dlist_create();

    // Change to the manager thread context and call the scheduler function
    // ucontext_t scheduler_ctx;
    if(getcontext(&scheduler.main_ctx) == -1) {
        printf("Error while getting context\n");
        exit(EXIT_FAILURE);
    }
    dccthread_create("main", func, param);

    // While there are threads to be computed
    while(scheduler.threads_list->count) {
        struct dnode* cur = scheduler.threads_list->head;
        // Iterate over thread lists
        while(cur) {
            dccthread_t* curThread = cur->data;
            // Set some flags to indicate the current thread being used
            curThread->state = RUNNING;
            scheduler.current_thread = curThread;

            // Execute the thread function
            swapcontext(&scheduler.main_ctx, &curThread->t_context);

            // Reset the flags
            scheduler.current_thread = NULL;
            // Remove this thread from the list and if the thread hasn't
            // finished, puts in the end (least priority)
            dlist_remove_from_node(scheduler.threads_list, cur);
            if(curThread->state == RUNNABLE)
                dlist_push_right(scheduler.threads_list, curThread);

            break;

            cur = cur->next;
        }
    }

    exit(EXIT_SUCCESS);
}

/**
 * @brief Creates a dcc thread
 *
 * @param name The name of the thread.
 * @param func The callback function that the thread is going to execute.
 * @param param The parameter to be passed into the callback function.
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
    new_thread->t_context.uc_link = &scheduler.main_ctx;
    new_thread->t_context.uc_stack.ss_sp = stack;
    new_thread->t_context.uc_stack.ss_size = THREAD_STACK_SIZE;
    new_thread->t_context.uc_stack.ss_flags = 0;
    // Make sure that when the context is swapped the <func> is called with
    // <param> parametter
    makecontext(&new_thread->t_context, (void (*)())func, 1, param);
    // Add this thread to the end of the list of waiting threads
    dlist_push_right(scheduler.threads_list, new_thread);
    //
    return new_thread;
}

/**
 * @brief Function that makes a thread yield and comeback to the scheduler.
 *
 */
void dccthread_yield(void) {
    dccthread_t* current_thread = dccthread_self();
    current_thread->state = RUNNABLE;
    // Swap back to the scheduler context
    swapcontext(&current_thread->t_context, &scheduler.main_ctx);
}

/**
 * @brief Function that returns the current thread being executed.
 *
 * @return dccthread_t* the current thread being executed.
 */
dccthread_t* dccthread_self(void) { return scheduler.current_thread; }

/**
 * @brief Function that returns the name of some thread.
 *
 * @param tid Thread to have its name returned.
 * @return const char* The name of the thread.
 */
const char* dccthread_name(dccthread_t* tid) { return tid->t_name; }