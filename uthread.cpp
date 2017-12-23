#include <iostream>
#include <sys/ucontext.h>
#include "uthread.h"
#include <sys/time.h>
#include <signal.h>
#include <cstring>
#include <algorithm>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include "stdio.h"

using namespace std;

int id_gen = 0;
// TCB Queue
TCB *run_TCB = NULL;

vector<TCB> sch_queue;

vector<TCB> suspend_queue;

vector<TCB> comp_queue;

struct sigaction sa;
struct itimerval timer;

ucontext_t cons[MAX_THREADS];

char * buffer[1000];

void setup_mainTCB() {
    // Sch queue will be empty here.
    TCB *current_TCB = new TCB;
    current_TCB->tid = id_gen++;
    current_TCB->st = RUNNING;
    run_TCB = current_TCB;
    getcontext(&(cons[current_TCB->tid]));
    cout << "Main TCB is setup" << endl;
    uthread_init(GLOBAL_TIME_SLICE);
}

int stub(){
   void *result = run_TCB->function(run_TCB->args);

   run_TCB->return_data = result;
    uthread_terminate(run_TCB->tid);
    return 0;
}

int uthread_create(void * (*start_routine)(void *),void *arg) {
    if (id_gen == 0) {
        setup_mainTCB();
    }
    disable_timer();


    TCB *current_TCB = new TCB;
    current_TCB->tid = id_gen++;
    current_TCB->st = READY;


    char *stack = (char *)malloc(STACK_SIZE);

    if(getcontext(&(cons[current_TCB->tid])) == -1) {
        cerr << "getcontext failed in uthread create" << endl;
        return -1;
    }

    // Context Things
    address_t sp, pc;
    pc = (address_t)stub;
    (cons[current_TCB->tid]).uc_mcontext.gregs[REG_RIP] = pc;

    sp = (address_t)stack + STACK_SIZE - sizeof(int);
    (cons[current_TCB->tid]).uc_mcontext.gregs[REG_RSP] = sp;

    // Modify the context to a new stack
    (cons[current_TCB->tid]).uc_link = 0;
    (cons[current_TCB->tid]).uc_stack.ss_flags = 0;

    sigemptyset(&cons[current_TCB->tid].uc_sigmask);     // SIGNALS

    current_TCB->function = start_routine;

    current_TCB->args = arg; // Put this in stack
    current_TCB->return_data = NULL; //Send pointer for thread to write in

    // Put current_TCB at the end of queue
    sch_queue.push_back(*current_TCB);


    cout << "TCB Thread --"  << id_gen - 1 <<  " is setup" << endl;
    enable_timer();
    return current_TCB->tid;
}

int uthread_self(void) {
    return run_TCB->tid;
}

void swapContext() {
    disable_timer();

    volatile int flag = 0;

    run_TCB->st = READY; // Make it running to ready
    getcontext(&(cons[run_TCB->tid]));

    if(flag == 1) {
        flag = 0;
        return;
    }

    sch_queue.push_back(*run_TCB);
    run_TCB = new TCB(sch_queue.front());
    run_TCB->st = RUNNING;
    sch_queue.erase(sch_queue.begin()); //pop

    flag = 1;

    enable_timer();

    setcontext(&(cons[run_TCB->tid]));

}

void uthread_yield() {

    disable_timer();
    if (sch_queue.empty()){
        cout << "​uthread_yield -- Nothing in Queue" << endl;
        exit(1);
    }

    swapContext();

    return;

}

int checkTID(int tid , vector<TCB> queue) {
    int found = -1;
    for ( int i = 0 ; i < queue.size() ; i++) {
        if (queue[i].tid == tid) {
            found = i;
            break;
        }
    }
    return found;
}

int uthread_join(int tid, void **retval){
    disable_timer();
    run_TCB->waiting_on_tid = tid;

    while (1){

        int index = checkTID(tid , comp_queue);
        if (index == -1 ) {
            if (checkTID(tid , sch_queue) != -1 || checkTID(tid , suspend_queue) != -1) {
                enable_timer();
                uthread_suspend(uthread_self());
            } else {
                break;
            }
        } else {
            *retval = comp_queue[index].return_data;
            comp_queue.erase(comp_queue.begin()+index);
            run_TCB->waiting_on_tid = -1;
            enable_timer();
            return 0;
        }

    }
    enable_timer();
    return -1;
}

void timer_handler (int signum)
{
    static int count = 0;
    cout << "Timer expired "<<++count << " times" << endl;

    //swap context should be called
    swapContext();
}

int uthread_init(int time_slice) {
    cout << "called init" << endl;
    /* Install timer_handler as the signal handler for SIGVTALRM. */
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;
    sigaction (SIGVTALRM, &sa, NULL);

    /* Configure the timer to expire after 'time_slice' msec... */
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = time_slice;

    //and every 'time_slice' msec after that.
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = time_slice;

    enable_timer();
}

void enable_timer() {
    setitimer(ITIMER_VIRTUAL,&timer,0);
}

void disable_timer() {
    setitimer(ITIMER_VIRTUAL, 0, 0);
}

void wakeUpCall(int terminated_tid) {
    for(int index = 0; index < suspend_queue.size(); index++) {
        if(suspend_queue[index].waiting_on_tid == terminated_tid) {
            uthread_resume(suspend_queue[index].tid);
        }
    }
}

int uthread_terminate(int tid) {
    disable_timer();

    // special case - when the running thread calls suspend on itself
    if(run_TCB->tid == tid) {
        comp_queue.push_back(*run_TCB);
        run_TCB = NULL;

        if(sch_queue.size() != 0) {

            run_TCB = new TCB(sch_queue.front());
            run_TCB->st = RUNNING;
            sch_queue.erase(sch_queue.begin());

            wakeUpCall(tid);
            enable_timer();
            setcontext(&(cons[run_TCB->tid]));
        }

        wakeUpCall(tid);

        enable_timer();
        return 0;

    }

    int index = checkTID(tid,sch_queue);

    if(index != -1) {
        //erase the thread from the queue if found
        sch_queue.erase(sch_queue.begin() + index);
        wakeUpCall(tid);
        enable_timer();
        return 0;
    } else {

        //search for the tid in the suspend queue
        index = checkTID(tid, suspend_queue);

        if(index != -1) {
            // if the  is found in the suspend queue delete the thread
            suspend_queue.erase(suspend_queue.begin() + index);
            wakeUpCall(tid);
            enable_timer();
            return 0;
        } else {
            //if the tid is not found in suspend queue
            cerr << "Tid : "<< tid << "is not present in the suspend queue";
        }

    }
    enable_timer();
    return -1;
}

int uthread_suspend(int tid) {
    disable_timer();
    cout << "Suspending thread with tid : " << tid << endl;
    // special case - when the running thread calls suspend on itself
    if(run_TCB->tid == tid) {
        volatile int check = 0;
        getcontext(&(cons[run_TCB->tid]));
        if (check == 1) {
            return 0;
        }
        //put it in the suspend queue
        run_TCB->st = SUSPENDED;
        suspend_queue.push_back(*run_TCB);

        //and then remove from the schedule queue and run
        run_TCB = new TCB(sch_queue.front());
        run_TCB->st = RUNNING;
        sch_queue.erase(sch_queue.begin()); //pop

        check = 1;
        enable_timer();
        // call set context
        setcontext(&(cons[run_TCB->tid]));
    }

    int index = checkTID(tid, sch_queue);

    if(index != -1) {
        //if the requested tid is present in the schedule queue
        //put it in the suspend queue
        suspend_queue.push_back(sch_queue[index]);

        //and then remove from the schedule queue
        sch_queue.erase(sch_queue.begin() + index);
    } else {
        //if the requested tid is not present in te schedule queue
        cerr << "tid : " << tid << " is not present in the schedule queue";
        enable_timer();
        return -1;
    }

    enable_timer();
    return 0;

}

int uthread_resume(int tid) {
    disable_timer();
    cout << "Resuming thread with tid : " << tid << endl;
    int index = checkTID(tid, suspend_queue);
    if(index != -1) {
        //if the requested tid is present in the schedule queue
        //put it in the suspend queue
        TCB *temp = new TCB(suspend_queue[index]);

        suspend_queue.erase(suspend_queue.begin() + index);

        if(sch_queue.size() != 0) {
            sch_queue.push_back(*temp);
        } else {
            run_TCB = new TCB(*temp);
            setcontext(&(cons[run_TCB->tid]));
        }

    } else {
        //if the requested tid is not present in te schedule queue
        cerr << "tid : " << tid << " is not present in the suspend queue";
        enable_timer();
        return -1;
    }
    enable_timer();
    return 0;
}

ssize_t async_read(int fildes, void *buf , size_t nbytes) {
    disable_timer();
    run_TCB->read_acy = new aiocb;

    memset(run_TCB->read_acy, 0, sizeof(aiocb));
    run_TCB->read_acy->aio_nbytes = nbytes;
    run_TCB->read_acy->aio_fildes = fildes;
    run_TCB->read_acy->aio_offset = 0;
    run_TCB->read_acy->aio_buf = buf;

    aio_read(run_TCB->read_acy);

    // wait until the request has finished
    while(aio_error(run_TCB->read_acy) == EINPROGRESS)
    {
        cout << "Still reading..." << endl;
        // Suspend
        enable_timer();
        uthread_yield();
    }
    cout << buf << endl;
    delete run_TCB->read_acy;
    run_TCB->read_acy = NULL;
    enable_timer();
    return 0;
}