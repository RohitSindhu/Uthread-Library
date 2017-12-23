#ifndef UNTITLED_UTHREAD_H
#define UNTITLED_UTHREAD_H

#endif //UNTITLED_UTHREAD_H
// User Thead Lib
#include <iostream>
#include <vector>
#include <ucontext.h>
#include <cstdlib>
#include <aio.h>

#define MAX_THREADS 100
#define STACK_SIZE 40960

using namespace std;

typedef unsigned long address_t;

enum STATE {
    READY,
    RUNNING,
    SUSPENDED,
    TERMINATED
};

struct TCB {
    TCB() {
    }
    TCB(const TCB& from) {
        tid = from.tid;
        waiting_on_tid = from.waiting_on_tid;
        st = from.st;
        args = from.args;
        return_data = from.return_data;
        function = from.function;
        read_acy = from.read_acy;
    }
    int tid;
    int waiting_on_tid = -1;
    // Thread State
    STATE st;

    aiocb *read_acy = NULL;

    // Entry Fucntion Info
    void *args = NULL;
    void *return_data = NULL;
    void * (*function)(void *);
};


const int GLOBAL_TIME_SLICE = 250000; // set the time slice to 25 msecs

// ________________________________________________________________________
// Library Functions
void setup_mainTCB();

int uthread_create(void * (*start_routine)(void *),void *arg);

void uthread_yield();

int uthread_self(void);

int uthread_join(int tid, void **retval);


int uthread_terminate(int tid);

int uthread_init(int time_slice);

int uthread_suspend(int tid);

int uthread_resume(int tid);

ssize_t async_read(int fildes, void *buf , size_t nbytes);

// ________________________________________________________________________

void disable_timer();

void enable_timer();