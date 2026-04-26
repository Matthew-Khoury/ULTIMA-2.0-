// Code Framework built from Lab 10
//Primary Author: Melanie Priestly 


#ifndef SCHEDULER_H //prevents the header from being passed into a program more than once
#define SCHEDULER_H

#define MAX_TASKS 3

#include <iostream>
#include <string>
#include <cstdint>
#include <vector>
#include "Queue.h"
#include "IPC.h"

using namespace std;

extern const string READY;  //"extern" implies that the definition of the variable is somewhere else, in this case the .cpp file
extern const string RUNNING;
extern const string BLOCKED;
extern const string DEAD;

class Semaphore;  // forward declaration
class ipc;        // forward declaration

struct task_memory_region {
    int handle;
    int size;
    int cursor;
};

struct tcb {
    int task_id;
    std::string state;
    clock_t start_time;
    tcb* next;

    // add mailbox pointer
    Queue<ipc::Message*> mailbox;
    Semaphore* mailbox_sema = nullptr;

    // add memory tracking
    std::vector<task_memory_region> memory_regions;
    int active_region_index = -1;
};

class scheduler{
    tcb* current;
    tcb* tail;
    long current_quantum;
    int next_available_task_id;

public:
    tcb task_table[MAX_TASKS];

    scheduler();
    ~scheduler();

    void set_quantum(long quantum);
    long get_quantum();

    void set_state(int the_taskid, string the_state);
    string get_state(int the_taskid);

    int get_task_id();
    int get_task_count();
    clock_t get_elapsed_time(int the_taskid);

    int create_task();
    void start();
    void yield ();

    void dump();
    void kill();
    void garbage();

    tcb *get_current();

    tcb* get_task(int task_id) {
        if (task_id >= 0 && task_id < MAX_TASKS)
            return &task_table[task_id];
        return nullptr;
    }

    const tcb* get_task(int task_id) const {
        if (task_id >= 0 && task_id < MAX_TASKS)
            return &task_table[task_id];
        return nullptr;
    }
};

#endif