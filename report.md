# PAGINADOR DE MEMÓRIA - RELATÓRIO

## 1. Termo de compromisso

  Os membros do grupo afirmam que todo o código desenvolvido para este trabalho é de autoria própria.  Exceto pelo material listado no item 3 deste relatório, os membros do grupo afirmam não ter copiado material da Internet nem ter obtido código de terceiros.

## 2. Membros do grupo e alocação de esforço

  Preencha as linhas abaixo com o nome e o e-mail dos integrantes do grupo.Substitua marcadores `XX` pela contribuição de cada membro do grupo no desenvolvimento do trabalho (os valores devem somar 100%).

  * Júnio Veras de Jesus Lima <junioveras127@gmail.com> 50%
  * Vinícius Braga Freire <vinciusb15999@gmail.com> 50%

## 3. Referências bibliográficas

  https://man7.org/linux/man-pages/
  https://pubs.opengroup.org/onlinepubs/007904875/idx/index.html

## 4. Estruturas de dados

   1. Descreva e justifique as estruturas de dados utilizadas para
     gerência das threads de espaço do usuário (partes 1, 2 e 5).


```c
//Uma enumeração de todos os possíveis estados da thread, usado para indicar quando a thread está rodando, quando é rodável, quando está esperando outra ou quando está dormindo.
enum u_int8_t { RUNNING, RUNNABLE, WAITING, SLEEPING } THREAD_STATE;

// Uma estrutura que contém as informações de uma thread
struct dccthread {
    char t_name[DCCTHREAD_MAX_NAME_SIZE];     // O nome da thread
    enum u_int8_t state;                      // O estado em que ela está
    ucontext_t t_context;                     // O contexto da thread
    dccthread_t* t_waiting;                   // Uma possível thread que ela poderia estar esperando quando está no estado WAITING
};

// Uma estrutura que contém as informações do escalonador para fazer a gerência de threads
struct scheduler {
    ucontext_t ctx;                          // O contexto do escalonador usado para retornar a ele
    struct dlist* threads_list;              // A lista de threads para gerenciar a execução e a ordem das threads, de forma a tratarmos os estados de cada thread
    dccthread_t* current_thread;             // A thread que está sendo analisada no momento
    
    struct itimerspec timer_interval;        // O valor de intervalo de tempo do timer para preempção, que será inicializado com os 10ms
    timer_t timer_id;                        // O id do timer usado para preempção
    struct sigevent sev;                     // O sinal de evento do timer usado para preempção, que será inicializado com os dados do evento (qual o timer e qual sinal ele vai emitir)
    struct sigaction sa;                     // O sinal de ação do timer usado para preempção, que será inicializado com a função que será chamada quando o sinal do timer for capturado
    sigset_t signals_set;                    // Um set de sinais usado para preempção, que será inicializado com o PRE_EMPTION_SIG
    u_int64_t n_waiting;                     // O número de threads que estão esperando por outra
    u_int64_t n_exited;                      // O número de threads completas que não foram alvo da função waiting
};
```

  2. Descreva o mecanismo utilizado para sincronizar chamadas de
     dccthread_yield e disparos do temporizador (parte 4).

     Para o sincronismo funcionar foi utilizado o `sigprocmask` para fazer o bloqueio do sinal (que está armazenado no set de sinais dentro da estrutura `scheduler` no campo `signals_set`) do temporizador ao entrar na função `yield`, e desbloquear o sinal logo após o contexto da thread voltar a ser chamado, que seria na linha seguinte de `swapcontext`. Essa mesma ideia foi utilizada nas outras funções (`wait`, `exit`, `create` e `sleep`). Durante a execução do escalonador (descrito na função `dccthread_init`) o sinal do temporizador está sempre bloqueado.