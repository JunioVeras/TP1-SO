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
enum u_int8_t { RUNNING, RUNNABLE, WAITING } THREAD_STATE;

/**
 * @brief A struct that defines a DCC thread.
 *
 */
struct dccthread {
    const char* t_name;
    enum u_int8_t state;
    ucontext_t t_context;
    dccthread_t* t_waiting;
};

/**
 * @brief A struct that holds all the scheduler main infos.
 *
 */
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
    //-------------- Timer infos -----------------------------------------------
    /**
     * @brief Timer interval value.
     *
     */
    struct itimerspec timer_interval;
    /**
     * @brief Timer id.
     *
     */
    timer_t timer_id;
    /**
     * @brief The timer signal event struct.
     *
     */
    struct sigevent sev;
    /**
     * @brief Timer signal action.
     *
     */
    struct sigaction sa;
    /**
     * @brief Set of all signs used by the scheduler.
     *
     */
    sigset_t set;
};

static scheduler_t scheduler;

typedef void (*callback_t)(int);

/**
 * @brief Configures the scheduler timer.
 *
 */
void configure_timer(void);
/**
 * @brief Timer handler for thread pre-emption.
 *
 */
void timer_handler(int);

/* -------------------------------------------------------------------------- */

void dccthread_init(void (*func)(int), int param) {
    // Create the list to hold all the threads managed by the scheduler
    scheduler.threads_list = dlist_create();
    // scheduler.main_ctx.uc_sigmask
    // Change to the manager thread context and call the scheduler function
    // ucontext_t scheduler_ctx;
    if(getcontext(&scheduler.main_ctx) == -1) {
        printf("Error while getting context\n");
        exit(EXIT_FAILURE);
    }
    dccthread_create("main", func, param);

    // Configure the timer
    configure_timer();

    // While there are threads to be computed
    while(scheduler.threads_list->count) {
        struct dnode* cur = scheduler.threads_list->head;
        // Iterate over thread lists
        while(cur) {
            dccthread_t* curThread = cur->data;
            if(curThread->state != WAITING) {
                // Set some flags to indicate the current thread being used
                curThread->state = RUNNING;
                scheduler.current_thread = curThread;

                // Execute the thread function
                sigprocmask(SIG_UNBLOCK, &scheduler.set, NULL);
                swapcontext(&scheduler.main_ctx, &curThread->t_context);
                sigprocmask(SIG_BLOCK, &scheduler.set, NULL);

                // If thread was deleted
                if(scheduler.current_thread != NULL) {
                    // Reset the flags
                    scheduler.current_thread = NULL;
                    // Remove this thread from the list and if the thread hasn't
                    // finished, puts in the end (least priority)
                    dlist_remove_from_node(scheduler.threads_list, cur);
                    if(curThread->state != RUNNING)
                        dlist_push_right(scheduler.threads_list, curThread);
                }
                break;
            }

            cur = cur->next;
        }
    }
    // Delete the timer
    timer_delete(scheduler.timer_id);

    exit(EXIT_SUCCESS);
}

dccthread_t* dccthread_create(const char* name, void (*func)(int), int param) {
    // Block timer signal
    sigprocmask(SIG_BLOCK, &scheduler.set, NULL);

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

    // Unblock timer signal
    sigprocmask(SIG_UNBLOCK, &scheduler.set, NULL);
    return new_thread;
}

void dccthread_yield(void) {
    // Block timer signal
    sigprocmask(SIG_BLOCK, &scheduler.set, NULL);

    dccthread_t* current_thread = dccthread_self();
    current_thread->state = RUNNABLE;

    // Unblock timer signal
    // sigprocmask(SIG_UNBLOCK, &scheduler.set, NULL);
    // Swap back to the scheduler context
    swapcontext(&current_thread->t_context, &scheduler.main_ctx);
}

void dccthread_exit(void) {
    // Block timer signal
    sigprocmask(SIG_BLOCK, &scheduler.set, NULL);

    dccthread_t* cur_thread = dccthread_self();
    struct dnode* cur = scheduler.threads_list->head;
    //
    while(cur) {
        dccthread_t* t = cur->data;
        if(t == cur_thread) {
            // Make sure to release the waiting threads
            if(t->t_waiting) {
                t->t_waiting->state = RUNNABLE;
            }
            scheduler.current_thread = NULL;
            // Removes node from the list
            dlist_remove_from_node(scheduler.threads_list, cur);
            // Removes this thread
            free(cur_thread);
            ucontext_t local_ctx;
            // Unblock timer signal
            // sigprocmask(SIG_UNBLOCK, &scheduler.set, NULL);

            swapcontext(&local_ctx, &scheduler.main_ctx);
            return;
        }
        //
        cur = cur->next;
    }

    // Unblock timer signal
    // sigprocmask(SIG_UNBLOCK, &scheduler.set, NULL);
}

void dccthread_wait(dccthread_t* tid) {
    // Block timer signal
    sigprocmask(SIG_BLOCK, &scheduler.set, NULL);

    dccthread_t* curThread = dccthread_self();

    // Search for the thread to be awaited
    struct dnode* cur = scheduler.threads_list->head;
    while(cur) {
        dccthread_t* t = cur->data;
        // If it's the thread to be awaited
        if(t == tid) {
            curThread->state = WAITING;
            t->t_waiting = curThread;
            // Unblock timer signal
            // sigprocmask(SIG_UNBLOCK, &scheduler.set, NULL);

            swapcontext(&curThread->t_context, &scheduler.main_ctx);
            return;
        }
        //
        cur = cur->next;
    }

    // Unblock timer signal
    sigprocmask(SIG_UNBLOCK, &scheduler.set, NULL);
}

dccthread_t* dccthread_self(void) { return scheduler.current_thread; }

const char* dccthread_name(dccthread_t* tid) { return tid->t_name; }

void configure_timer() {
    // Define timer interval of 10ms
    scheduler.timer_interval.it_interval.tv_nsec = 10000000;
    scheduler.timer_interval.it_interval.tv_sec = 0;
    scheduler.timer_interval.it_value = scheduler.timer_interval.it_interval;
    // Define timer signal event
    scheduler.sev.sigev_value.sival_ptr = &scheduler.timer_interval;
    scheduler.sev.sigev_notify = SIGEV_SIGNAL;
    scheduler.sev.sigev_notify_attributes = NULL;
    scheduler.sev.sigev_signo = SIGUSR1;
    scheduler.sev.sigev_notify_function = (void (*)(__sigval_t))timer_handler;
    // Setting the signal handlers before invoking timer
    scheduler.sa.sa_handler = timer_handler;
    scheduler.sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGUSR1, &scheduler.sa, NULL);
    // Set the set of signs
    sigemptyset(&scheduler.set);
    sigaddset(&scheduler.set, SIGUSR1);
    // Create timer
    if(timer_create(
           CLOCK_PROCESS_CPUTIME_ID, &scheduler.sev, &scheduler.timer_id)
       == -1) {
        printf("Error while creating timer\n");
        exit(EXIT_FAILURE);
    }
    // Start timer
    timer_settime(scheduler.timer_id, 0, &scheduler.timer_interval, NULL);
    // Block timer signal
    sigprocmask(SIG_BLOCK, &scheduler.set, NULL);
}

void timer_handler(int signal) {
    // Stops the current thread
    puts("aaaaaaaaaaaa");
    dccthread_yield();
}
