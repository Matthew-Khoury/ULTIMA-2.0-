#ifndef SCHEDULER.h //prevents the header from being passed into a program more than once 
#define SCHEDULER.h 

#include <iostream>
#include <string>
#include <ctime>
#include "Queue.h"

using namespace std; 

extern const string READY = "Ready";    //"extern" implies that the defintion of the variable is somewhere else, in thsi case the .cpp file
extern const string RUNNING = "Running";
extern const string BLOCKED = "Blocked"; 
extern const string DEAD = "Dead";

const int MAX_TASKS = 3; //how do we determine the number of tasks allowed?

struct tcb {
    int task_id;
    string state;
    clock_t start_time;
    tcb *next;

};

class scheduler{
   
    tcb* current;
    tcb* tail;
    long current_quantum;
    int next_available_task_id;
    tcb task_table[MAX_TASKS]; 

public:

    scheduler();
    ~scheduler();

    void set_quantum(long quantum);
    long get_quantum();

    void set_state(int the_taskid, string the_state);
    int get_task_id();

    int create_task();
    void start();
    void yield ();

    void dump();

};

