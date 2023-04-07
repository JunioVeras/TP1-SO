#include <stdio.h>
#include <stdlib.h>
#include "dccthread.h"

void test3(int dummy) {
    printf("entered %s with parameter %d\n", __func__, dummy);
    dccthread_exit();
}

void test2(int dummy) {
    dccthread_create("thread3", test3, dummy * 2);
    dccthread_exit();
    printf("entered %s with parameter %d\n", __func__, dummy);
}

void test(int dummy) {
    printf("entered %s with parameter %d\n", __func__, dummy);
    dccthread_create("thread2", test2, dummy * 2);
    dccthread_exit();
}

int main(int argc, char** argv) { dccthread_init(test, 2); }
