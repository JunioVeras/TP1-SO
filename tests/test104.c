#include <stdio.h>
#include <stdlib.h>
#include "dccthread.h"

#define NUM_THREADS 20
dccthread_t* threads[NUM_THREADS];
int nThreads;

void waitOrNot(int i) {
    if(i < NUM_THREADS) {
        printf("Num of threads waiting: %d.\n", dccthread_nwaiting());
        if(!(i % 2)) {
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

// Função de teste para dccthread_nwaiting
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
        "sao %d threads esperando\n",
        dccthread_name(dccthread_self()),
        cont,
        dccthread_nwaiting());
    dccthread_exit();
}

int main(int argc, char** argv) { dccthread_init(test, 7); }
