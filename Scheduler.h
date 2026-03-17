#ifndef SCHEDULER_H //prevents the header from being passed into a program more than once
#define SCHEDULER_H

#include <iostream>
#include <string>
#include <ctime>

using namespace std; 

extern const string READY;  //"extern" implies that the defintion of the variable is somewhere else, in thsi case the .cpp file
extern const string RUNNING;
extern const string BLOCKED;
extern const string DEAD;

const int MAX_TASKS = 3; //how do we determine the number of tasks allowed?

struct tcb {
    int task_id;
    string state;
    clock_t start_time;
    tcb *next;  //this line creates a pointer to the next node in the linked list
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
    string get_state(int the_taskid);

    int get_task_id();

    int create_task();
    void start();
    void yield ();

    void dump();

};

#endif
