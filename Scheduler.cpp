#include "Scheduler.h"
#include <unistd.h>   // for sleep()

const string READY   = "Ready";
const string RUNNING = "Running";
const string BLOCKED = "Blocked";
const string DEAD    = "Dead";

scheduler::scheduler() {
    current = nullptr;
    tail = nullptr;
    next_available_task_id = 0;   //start the system start at 0
    current_quantum = 300;
}

scheduler::~scheduler() {
    // cout << "Exiting Scheduler..." << endl;
}

void scheduler::set_quantum(long quantum) {
    current_quantum = quantum;
}

long scheduler::get_quantum() {
    return current_quantum;
}

void scheduler::set_state(int the_taskid, string the_state) {
    task_table[the_taskid].state = the_state;
}

string scheduler::get_state(int the_taskid) {
    return task_table[the_taskid].state;
}

int scheduler::get_task_id() {
    return (current != nullptr) ? current->task_id : -1;  // will return the current task
}

int scheduler::get_task_count() {
    return next_available_task_id;
}

clock_t scheduler::get_elapsed_time(int the_taskid) {
    return clock() - task_table[the_taskid].start_time;
}

int scheduler::create_task() {
    if (next_available_task_id >= MAX_TASKS) {
        // cout << "Create_task FAILED: MAX_TASKS exceeded." << endl;
        return -1; //return error
    }

    int id = next_available_task_id;
    // cout << "Creating task #" << id << endl;

    tcb* new_task = &task_table[id];
    new_task->task_id = id;
    new_task->state = READY;
    new_task->next = nullptr;

    // Build circular linked list
    if (current == nullptr) {
        current = new_task;
        tail = new_task;
        tail->next = current;
    } else {
        tail->next = new_task;
        tail = new_task;
        tail->next = current;
    }

    next_available_task_id++;
    return id;
}

void scheduler::start() {
    // cout << "-----------------------------" << endl;
    // cout << "       SCHEDULER STARTED     " << endl;
    // cout << "-----------------------------" << endl;

    if (!current) {
        // cout << "No tasks to schedule!" << endl;
        return;
    }

    current->start_time = clock();
    current->state = RUNNING;

    set_quantum(1000 / MAX_TASKS);  //setting the fixed amount of time the CPU will allocate for the process

    sleep(1);
}

void scheduler::yield() {
    if (!current) return;

    // cout << "Task #" << current->task_id << " attempting to yield..." << endl;

    clock_t elapsed = clock() - current->start_time;
    // cout << "Elapsed time: " << elapsed << endl;

    if (elapsed < current_quantum) {
        // cout << "NO YIELD — quantum remaining." << endl;
        return;
    }

    // cout << "Yielding... switching tasks." << endl;

    //if running, we should change it to ready since the quantum time has run out
    if (current->state == RUNNING)
        current->state = READY;

    // current_task = (current_task +1) % MAX_TASKS; from lab 12 replaced by the following line
    current = current->next;

    int counter = 0;
    while (current->state != READY && counter < MAX_TASKS-1) { //we are subtracting one because we can not surpase the max tasks
        current = current->next;
        counter++;
    }

    if (counter >= MAX_TASKS) {
        // cout << "DEADLOCK DETECTED — no READY tasks." << endl;
        return;
    }

    current->start_time = clock();
    current->state = RUNNING;

    // cout << "Now running task #" << current->task_id << endl;
}

void scheduler::dump() {
    cout << "---------------- PROCESS TABLE ----------------" << endl;
    cout << "Quantum = " << current_quantum << endl;
    cout << "Task ID\tElapsed\tState" << endl;

    for (int i = 0; i < next_available_task_id; i++) {
        clock_t elapsed_time = clock() - task_table[i].start_time;

        printf("%2d\t%6d\t%s",
               task_table[i].task_id,
               (int)elapsed_time,
               task_table[i].state.c_str());

        if (current && i == current->task_id) // ** ask the group to
            cout << "  <--- CURRENT PROCESS";

        cout << endl;
    }

    cout << "------------------------------------------------" << endl;

}
void scheduler:: kill(){
    if(!current)return;
    current.state = DEAD;
    yield(); // proceed to the next task 

}
void scheduler :: garbage(){
    if(!current || next_available_task_id ==0) return;

    tcb* prev =tail; 
    tcb* curr = current;

    for (int i = 0; i < next_available_task_id; i++){
        if(curr->state ==DEAD){
            prev-> next = curr->next; //this skips the node we want to remove by have the previous task point to the next task
            if(tail == curr){ //since this a circular linked list, if the dead task is the tail, we will update the tail to be the previous task
                tail = prev;
            }  
            if(current == curr){
                current = curr->next; // updating where the current node is pointing to
            }      
        curr = prev->next;
        }else{
        prev = curr;
        curr = curr->next;
        }
    }

}

