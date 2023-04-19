#include <stdio.h>
#include <stdlib.h>
#include "dccthread.h"

#define NUM_THREADS 20
// Define a cada quantas thread 1 thread vai esperar outra
#define RATIO_OF_WAITING 3
dccthread_t* threads[NUM_THREADS];
int nThreads;

void waitOrNot(int i) {
    if(i < NUM_THREADS) {
        if(!(i % RATIO_OF_WAITING) && i != NUM_THREADS - 1) {
            int toWait = i + 1;
            printf("Thread-%d will wait for thread-%d.\n", i, toWait);
            dccthread_wait(threads[toWait]);
        }
        else {
            struct timespec ts;
            ts.tv_sec = 5;
            dccthread_sleep(ts);
        }
    }
    dccthread_exit();
}

// Função de teste para dccthread_nexited
void test(int cont) {
    for(int i = 0; i < NUM_THREADS; i++) {
        char str[16];
        sprintf(str, "thread-%d", i);
        threads[i] = dccthread_create(str, waitOrNot, i);
    }

    dccthread_yield();

    struct timespec ts;
    ts.tv_sec = 6;
    dccthread_sleep(ts);
    printf(
        "Thread %s executada aqui com argumento %d. Agora, após o fim de tudo, "
        "foram %d threads terminadas sem serem esperadas, afinal temos %d "
        "threads e a cada %d, 1 espera por outra.\n",
        dccthread_name(dccthread_self()),
        cont,
        dccthread_nexited(),
        NUM_THREADS,
        RATIO_OF_WAITING);
    dccthread_exit();
}

int main(int argc, char** argv) { dccthread_init(test, 7); }
