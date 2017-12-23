#include <iostream>
#include "uthread.h"

using namespace std;

void* t1(void *i){
    cout << "Starting Thread 1" << endl;
    int *arg = (int *)i;

    for ( int i = 0 ; i < 100 ; i++ ){
        for ( int j = 0 ; j < 1000000 ; j++ ){
            for ( int k = 0 ; k < 20 ; k++ ) {}
        }
        cout << "Argument passed in T1 : " << *arg << endl;
    }
    cout << "Ending Thread 1" << endl;
}

void* t2(void *i){
    cout << "Starting Thread 2" << endl;
    int *arg = (int *)i;
    for ( int i = 0 ; i < 100 ; i++ ){
        for ( int j = 0 ; j < 1000000 ; j++ ){
            for ( int k = 0 ; k < 20 ; k++ ) {}
        }
        cout << "Argument passed in T2 : " << *arg << endl;
    }
    cout << "Ending Thread 2" << endl;
}

void* t3(void *i){
    cout << "Starting Thread 3" << endl;
    int *arg = (int *)i;
    for ( int i = 0 ; i < 100 ; i++ ){
        for ( int j = 0 ; j < 1000000 ; j++ ){
            for ( int k = 0 ; k < 20 ; k++ ) {}
        }
        cout << "Argument passed in T3 : " << *arg << endl;
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
    int t2PID;

    void *ar2 = (void*)malloc(100);
    void **t2RetVal = (void**)&ar2;

    t2PID = uthread_create(t2, (void*)(&t2Arg));

    int t3Arg = 3;
    int t3PID;

    void *ar3 = (void*)malloc(100);
    void **t3RetVal = (void**)&ar3;

    t3PID = uthread_create(t3, (void*)(&t3Arg));

    uthread_join(t1PID,t1RetVal);
    cout << "t1 check crossed" << endl;
    uthread_join(t2PID,t2RetVal);
    cout << "t2 check crossed" << endl;
    uthread_join(t3PID,t3RetVal);
    cout << "t3 check crossed" << endl;

    cout << "Main done" << endl;

    return 0;
}