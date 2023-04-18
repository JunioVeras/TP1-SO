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

#define PRE_EMPTION_SIG SIGUSR1
#define SLEEP_SIGNAL SIGUSR2

/**
 * @brief An enumeration of all avaiable thread states.
 *
 */
enum u_int8_t { RUNNING, RUNNABLE, WAITING, SLEEPING } THREAD_STATE;

/**
 * @brief A struct that defines a DCC thread.
 *
 */
struct dccthread {
    char t_name[DCCTHREAD_MAX_NAME_SIZE];
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
    ucontext_t ctx;
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
     * @brief A signal set.
     *
     */
    sigset_t signals_set;
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
/**
 * @brief Function that handle the sleep timer event.
 *
 * @param signo The signal send to the timer event.
 * @param wrapped_info A info wrapper holding the `dccthread_t` that is going to
 * be awaken.
 * @param _
 */
void sleep_timer_handler(int signo, siginfo_t* wrapped_info, void* _);

/* -------------------------------------------------------------------------- */

void dccthread_init(void (*func)(int), int param) {
    // Create the list to hold all the threads managed by the scheduler
    scheduler.threads_list = dlist_create();
    // Create main thread
    dccthread_create("main", func, param);

    // Change to the manager thread context and call the scheduler function
    if(getcontext(&scheduler.ctx) == -1) {
        printf("Error while getting context\n");
        exit(EXIT_FAILURE);
    }

    // Configure the timer
    configure_timer();

    // While there are threads to be computed
    while(scheduler.threads_list->count) {
        struct dnode* cur = scheduler.threads_list->head;
        // Iterate over thread lists
        while(cur) {
            dccthread_t* curThread = cur->data;
            // Only execute RUNNABLE threads (WAITING and SLEEPING are ignored)
            if(curThread->state < WAITING) {
                // Set some flags to indicate the current thread being used
                curThread->state = RUNNING;
                scheduler.current_thread = curThread;

                // Execute the thread function
                swapcontext(&scheduler.ctx, &curThread->t_context);

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
    dccthread_t* new_thread = (dccthread_t*)malloc(sizeof(dccthread_t));
    // Instantiate the thread
    strcpy(new_thread->t_name, name);
    new_thread->state = RUNNABLE;
    new_thread->t_waiting = NULL;
    // Create a new context and stack
    if(getcontext(&new_thread->t_context) == -1) {
        puts("Error while getting context...exiting\n");
        exit(EXIT_FAILURE);
    }
    char* stack = (char*)malloc(THREAD_STACK_SIZE * sizeof(char));
    new_thread->t_context.uc_link = &scheduler.ctx;
    new_thread->t_context.uc_stack.ss_sp = stack;
    new_thread->t_context.uc_stack.ss_size = THREAD_STACK_SIZE;
    new_thread->t_context.uc_stack.ss_flags = 0;
    sigemptyset(&new_thread->t_context.uc_sigmask);
    sigprocmask(SIG_UNBLOCK, &scheduler.ctx.uc_sigmask, NULL);

    // Make sure that when the context is swapped the <func> is called with
    // <param> parametter
    makecontext(&new_thread->t_context, (void (*)())func, 1, param);
    // Add this thread to the end of the list of waiting threads
    dlist_push_right(scheduler.threads_list, new_thread);

    return new_thread;
}

void dccthread_yield(void) {
    sigprocmask(SIG_BLOCK, &scheduler.signals_set, NULL);
    scheduler.current_thread->state = RUNNABLE;
    // Swap back to the scheduler context
    swapcontext(&scheduler.current_thread->t_context, &scheduler.ctx);
    sigprocmask(SIG_UNBLOCK, &scheduler.signals_set, NULL);
}

void dccthread_exit(void) {
    sigprocmask(SIG_BLOCK, &scheduler.signals_set, NULL);
    //
    struct dnode* cur = scheduler.threads_list->head;
    while(cur) {
        dccthread_t* t = cur->data;
        if(t == scheduler.current_thread) {
            // Make sure to release the waiting threads
            if(t->t_waiting) {
                t->t_waiting->state = RUNNABLE;
            }
            // Removes node from the list
            dlist_remove_from_node(scheduler.threads_list, cur);
            // Removes this thread
            free(scheduler.current_thread);
            scheduler.current_thread = NULL;

            setcontext(&scheduler.ctx);
            // Unblock timer signal but unnecessary command
            sigprocmask(SIG_UNBLOCK, &scheduler.signals_set, NULL);
            return;
        }
        //
        cur = cur->next;
    }
    // Unreachable code
    puts(
        "Unreachable piece of code and unexpected error. Look at "
        "dccthread_exit source code.");
    exit(EXIT_FAILURE);
}

void dccthread_wait(dccthread_t* tid) {
    sigprocmask(SIG_BLOCK, &scheduler.signals_set, NULL);

    // Search for the thread to be awaited
    struct dnode* cur = scheduler.threads_list->head;
    while(cur) {
        dccthread_t* t = cur->data;
        // If it's the thread to be awaited
        if(t == tid) {
            scheduler.current_thread->state = WAITING;
            t->t_waiting = scheduler.current_thread;

            swapcontext(&scheduler.current_thread->t_context, &scheduler.ctx);
            // Unblock timer signal
            sigprocmask(SIG_UNBLOCK, &scheduler.signals_set, NULL);
            return;
        }
        //
        cur = cur->next;
    }
    // Unblock timer signal since the thread id doesn't exist
    sigprocmask(SIG_UNBLOCK, &scheduler.signals_set, NULL);
}

void sleep_timer_handler(int signo, siginfo_t* wrapped_info, void* _) {
    dccthread_t* thread = wrapped_info->si_value.sival_ptr;
    // Unwrap the info and turn the thread executable again
    thread->state = RUNNABLE;
}

void dccthread_sleep(struct timespec ts) {
    sigprocmask(SIG_BLOCK, &scheduler.signals_set, NULL);

    // Blocks the thread from execution
    scheduler.current_thread->state = SLEEPING;

    struct sigevent sev;
    timer_t timer_id;
    // Define timer signal event
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SLEEP_SIGNAL;
    sev.sigev_value.sival_ptr =
        scheduler
            .current_thread;  // Allow the timer handler to receive your thread
                              // point related to the correct timer event
    // Defines action on signal detection
    struct sigaction sa;
    sa.sa_sigaction = sleep_timer_handler;
    sa.sa_flags = SA_SIGINFO;  // Allow the user to pass more infos to the timer
    sa.sa_mask = scheduler.signals_set;  // Make sure all the signals are
                                         // blocked inside the handler
    sigaction(SLEEP_SIGNAL, &sa, NULL);
    // Create timer
    if(timer_create(CLOCK_REALTIME, &sev, &timer_id) == -1) {
        printf("Error while creating timer\n");
        exit(EXIT_FAILURE);
    }

    // Define timer interval to the desired one
    struct itimerspec time;
    time.it_value = ts;
    time.it_interval.tv_nsec = 0;
    time.it_interval.tv_sec = 0;
    // Start timer
    timer_settime(timer_id, 0, &time, NULL);

    // Swap back to the scheduler context
    swapcontext(&scheduler.current_thread->t_context, &scheduler.ctx);

    sigprocmask(SIG_UNBLOCK, &scheduler.signals_set, NULL);
}

dccthread_t* dccthread_self(void) { return scheduler.current_thread; }

const char* dccthread_name(dccthread_t* tid) { return tid->t_name; }

void configure_timer() {
    // Initializes signs blockers for timers
    sigemptyset(&scheduler.signals_set);
    sigaddset(&scheduler.signals_set, PRE_EMPTION_SIG);
    // Blocks timer for scheduler thread
    sigprocmask(SIG_BLOCK, &scheduler.signals_set, NULL);
    scheduler.ctx.uc_sigmask = scheduler.signals_set;
    // Define timer signal event
    scheduler.sev.sigev_value.sival_ptr = &scheduler.timer_id;
    scheduler.sev.sigev_notify = SIGEV_SIGNAL;
    scheduler.sev.sigev_signo = PRE_EMPTION_SIG;
    // Defines action on signal detection
    scheduler.sa.sa_handler = timer_handler;
    scheduler.sa.sa_flags = 0;
    sigaction(PRE_EMPTION_SIG, &scheduler.sa, NULL);
    // Create timer
    if(timer_create(
           CLOCK_PROCESS_CPUTIME_ID, &scheduler.sev, &scheduler.timer_id)
       == -1) {
        printf("Error while creating timer\n");
        exit(EXIT_FAILURE);
    }

    // Define timer interval of 10ms
    scheduler.timer_interval.it_interval.tv_nsec = 10000000;
    scheduler.timer_interval.it_interval.tv_sec = 0;
    scheduler.timer_interval.it_value = scheduler.timer_interval.it_interval;
    // Start timer
    timer_settime(scheduler.timer_id, 0, &scheduler.timer_interval, NULL);
}

void timer_handler(int signal) {
    // Stops the current thread
    dccthread_yield();
}
