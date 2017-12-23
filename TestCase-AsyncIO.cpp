#include <iostream>
#include <ucontext.h>
#include <zconf.h>
#include "uthread.h"
#include <fcntl.h>

using namespace std;

void* t1(void *i){

    cout << "Starting Thread 1" << endl;

    cout << "Doing async I/O" << endl;
    int file = open("test.txt", O_RDONLY, 0);
    if (file == -1) {
        cout << "Unable to open file!" << endl;
    }

    char* buffer = new char[1000];

    async_read(file, (void* )buffer, 1000);


    cout << "Printing File contents :- " << endl;
    cout << buffer << endl;

    delete[] buffer;
    close(file);
    cout << "Ending Thread 1" << endl;
}

void* t2(void *i){
    cout << "Starting Thread 2" << endl;

    for ( int i = 0 ; i < 10 ; i++ ){
        for ( int j = 0 ; j < 10000 ; j++ ){ }
        cout << "in T2" << endl;
    }
    cout << "Ending Thread 2" << endl;
}

void* t3(void *i){
    cout << "Starting Thread 3" << endl;

    for ( int i = 0 ; i < 10 ; i++ ){
        for ( int j = 0 ; j < 10000 ; j++ ){ }
        cout << "in T3" << endl;
    }
    cout << "Ending Thread 3" << endl;
}

int main() {
    // Put Main TCB in sch_queue
    int t1Arg = 1;

    int t1PID;
    void *ar1 = (void*)malloc(100);
    void **t1RetVal = (void**)&ar1;

    t1PID = uthread_create(t1, (void*)(&t1Arg));

    int t2Arg = 2;
    int t3Arg = 3;
    int t2PID, t3PID;

    void *ar2 = (void*)malloc(100);
    void **t2RetVal = (void**)&ar2;

    void *ar3 = (void*)malloc(100);
    void **t3RetVal = (void**)&ar3;

    t2PID = uthread_create(t2, (void*)(&t2Arg));
    t3PID = uthread_create(t3, (void*)(&t3Arg));


    uthread_join(t1PID,t1RetVal);
    cout << "t1 check crossed" << endl;
    uthread_join(t2PID,t1RetVal);
    cout << "t2 check crossed" << endl;
    uthread_join(t3PID,t1RetVal);
    cout << "t3 check crossed" << endl;

    cout << "Main done" << endl;

    return 0;
}