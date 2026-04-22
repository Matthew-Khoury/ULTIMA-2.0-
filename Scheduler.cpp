// Code Framework built from Lab 10
// Primary Author: Matthew Khoury
// Co-contributor Melanie Priestly

#include "Scheduler.h"
#include "Semaphore.h"
#include "IPC.h"
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

    // Initialize each tcb's mailbox semaphore with this scheduler
    for (int i = 0; i < MAX_TASKS; i++) {
        task_table[i].mailbox_sema = nullptr;
        task_table[i].memory_regions.clear();
        task_table[i].active_region_index = -1;
    }
}
//this is a practice change

scheduler::~scheduler() {
    // Delete mailbox semaphores
    for (int i = 0; i < next_available_task_id; i++) {
        if (task_table[i].mailbox_sema != nullptr) {
            delete task_table[i].mailbox_sema;
            task_table[i].mailbox_sema = nullptr;
        }
    }
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
    if (next_available_task_id >= MAX_TASKS) return -1;

    int id = next_available_task_id;
    tcb* new_task = &task_table[id];
    new_task->task_id = id;
    new_task->state = READY;
    new_task->next = nullptr;

    new_task->memory_regions.clear();
    new_task->active_region_index = -1;

    // Allocate mailbox semaphore immediately
    new_task->mailbox_sema = new Semaphore(1, "Mailbox", this);

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

    tcb* start = current;

    do {
        current = current->next;
        if (current->state == READY)
            break;
    } while (current != start);

    if (current->state != READY) {
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

    bool has_active = false;

    if (current) {
        tcb* temp = current;
        do {
            if (temp->state != DEAD) {
                has_active = true;
                break;
            }
            temp = temp->next;
        } while (temp != current);
    }

    if (!has_active) {
        cout << "No active tasks." << endl;
        cout << "------------------------------------------------" << endl;
        return;
    }

    tcb* temp = current;

    do {
        if (temp->state != DEAD) {
            clock_t elapsed_time = clock() - temp->start_time;

            printf("%2d\t%6d\t%s",
                   temp->task_id,
                   (int)elapsed_time,
                   temp->state.c_str());

            if (temp == current)
                cout << "  <--- CURRENT PROCESS";

            cout << endl;
        }

        temp = temp->next;

    } while (temp != current);

    cout << "------------------------------------------------" << endl;
}

void scheduler:: kill(){
    if(!current)return;
    current->state = DEAD;
    yield(); // proceed to the next task
}

void scheduler::garbage() {
    if (!current) return;

    tcb* first_alive = nullptr;
    tcb* last_alive = nullptr;

    tcb* start = current;
    tcb* node = current;

    do {
        tcb* next_node = node->next;

        if (node->state != DEAD) {
            if (first_alive == nullptr) {
                first_alive = node;
                last_alive = node;
            }
            else {
                last_alive->next = node;
                last_alive = node;
            }
        }

        node = next_node;
    } while (node != start);

    if (first_alive == nullptr) {
        current = nullptr;
        tail = nullptr;
        return;
    }

    last_alive->next = first_alive;
    current = first_alive;
    tail = last_alive;
}

tcb* scheduler::get_current() {
    return current;
}