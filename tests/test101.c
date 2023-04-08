#include <stdio.h>
#include <stdlib.h>
#include "dccthread.h"

void test_fast(int dummy) {
    printf("Thread %s executada aqui com argumento %d\n",
           dccthread_name(dccthread_self()),
           dummy);
    dccthread_exit();
}

void test_slow(int dummy) {
    dccthread_create("fast", test_fast, 2);
    int a = 0;
    for(int i = 0; i < 800000000; i++) {
        a++;
    }
    printf("Thread %s executada aqui com argumento %d\n",
           dccthread_name(dccthread_self()),
           dummy);
    dccthread_exit();
}

int main(int argc, char** argv) { dccthread_init(test_slow, 1); }
