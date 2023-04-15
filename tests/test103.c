#include <stdio.h>
#include <stdlib.h>
#include "dccthread.h"

void test_fast(int dummy) {
    int a = 0;
    for(int i = 0; i < 3; i++) {
        a++;
    }
    printf("Thread %s executada aqui com argumento %d\n",
           dccthread_name(dccthread_self()),
           dummy);
    dccthread_exit();
}

// Função de teste para testar caso onde wait é executado em thread q não existe
// mais
void test_slow(int dummy) {
    dccthread_t* fastT = dccthread_create("fast", test_fast, 2);
    dccthread_yield();

    for(int a = 0, i = 0; i < 3000000; i++) {
        a++;
    }
    // O timer deve ser desbloqueado para este contexto já que a thread a ser
    // esperada já nao existe mais. Assim, yields devem ser chamados no loop a
    // seguir devido a preempção
    dccthread_wait(fastT);
    for(int a = 0, i = 0; i < 5000000; i++) {
        a++;
    }

    printf("Thread %s executada aqui com argumento %d\n",
           dccthread_name(dccthread_self()),
           dummy);
    dccthread_exit();
}

int main(int argc, char** argv) { dccthread_init(test_slow, 1); }
