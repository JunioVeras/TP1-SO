#ifndef __DCCTHREAD_HEADER__
#define __DCCTHREAD_HEADER__

#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <ucontext.h>
#include "dlist.h"
#include "stdio.h"

typedef struct dccthread dccthread_t;
typedef struct scheduler scheduler_t;

#define DCCTHREAD_MAX_NAME_SIZE 256
#define THREAD_STACK_SIZE (1 << 16)

/**
 * @brief Function responsible for simulating a thread scheduler.
 *
 * @param func The function for the main thread to be spawned.
 * @param param Parameter to be passed to <func>
 */
void dccthread_init(void (*func)(int), int param) __attribute__((noreturn));

/**
 * @brief Creates a dcc thread.
 *
 * @param name The name of the thread.
 * @param func The callback function that the thread is going to execute.
 * @param param The parameter to be passed into the callback function.
 * @return dccthread_t*
 */
dccthread_t* dccthread_create(const char* name, void (*func)(int), int param);

/**
 * @brief Function that makes a thread yield and comeback to the scheduler.
 *
 */
void dccthread_yield(void);

/**
 * @brief Function that stops a thread execution flow and removes it from the
 * threads list
 *
 */
void dccthread_exit(void);

/**
 * @brief Function that makes the current thread wait for another one. If this
 * this thread doesn't exists, then this threads waits for nothing.
 *
 * @param tid Pointer to the thread to be waited.
 */
void dccthread_wait(dccthread_t* tid);

/* `dccthread_sleep` stops the current thread for the time period
 * specified in `ts`. */
void dccthread_sleep(struct timespec ts);

/**
 * @brief Function that returns the current thread being executed.
 *
 * @return dccthread_t* the current thread being executed.
 */
dccthread_t* dccthread_self(void);

/**
 * @brief Function that returns the name of some thread.
 *
 * @param tid Thread to have its name returned.
 * @return const char* The name of the thread.
 */
const char* dccthread_name(dccthread_t* tid);

#endif
