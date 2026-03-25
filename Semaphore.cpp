/* Code Framework built from Lab 10
Primary Author: Matthew Khoury*/

#include "Semaphore.h"
#include <iostream>
#include <unistd.h>

using std::cout;
using std::endl;

// Constructor
Semaphore::Semaphore(int initialValue, string name, scheduler* theScheduler) {
    sema_value   = initialValue;
    resource_name = name;
    lucky_task   = -1;

    pthread_mutex_init(&lock, nullptr);
    pthread_cond_init(&cond, nullptr);

    sema_queue = new Queue<int>();
    sched_ptr  = theScheduler;
}

// Destructor
Semaphore::~Semaphore() {
    delete sema_queue;

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
}

string Semaphore::get_resource_name() {
    return resource_name;
}

int Semaphore::get_sema_value() {
    return sema_value;
}

int Semaphore::get_lucky_task() {
    return lucky_task;
}

string Semaphore::get_queue_string() {
    if (sema_queue->isEmpty()) {
        return "(empty)";
    } else {
        return sema_queue->Get_Q_String();
    }
}

// down(): attempt to acquire the resource
void Semaphore::down(int taskID) {
    bool blocked_task = false;

    pthread_mutex_lock(&lock);

    // if task is already BLOCKED, ignore request
    if (sched_ptr->get_state(taskID) == BLOCKED) {
        pthread_mutex_unlock(&lock);
        return;
    }

    if (taskID == lucky_task) {
        // already owns resource then do nothing
    } else {
        if (sema_value > 0) {
            // Resource available
            sema_value--;
            lucky_task = taskID;
        } else {
            // No resource then block logically
            sema_queue->En_Q(taskID);
            sched_ptr->set_state(taskID, BLOCKED);
            blocked_task = true;
        }
    }

    pthread_mutex_unlock(&lock);

    // yield after unlocking to avoid deadlock
    if (blocked_task) {
        sched_ptr->yield();
    }
}

// up(): release the resource
void Semaphore::up() {
    pthread_mutex_lock(&lock);

    int current_id = sched_ptr->get_task_id();

    if (current_id == lucky_task) {   // only owner can release
        if (sema_queue->isEmpty()) {
            // No one waiting → resource becomes free
            sema_value = 1;
            lucky_task = -1;
        } else {
            // Someone waiting then transfer ownership
            int taskID = sema_queue->De_Q();
            sched_ptr->set_state(taskID, READY);

            // Give resource directly to next task
            sema_value = 0;
            lucky_task = taskID;
        }
    } else {
        // invalid release
    }

    pthread_mutex_unlock(&lock);
}

void Semaphore::dump() {
    pthread_mutex_lock(&lock);

    cout << "\nSemaphore Dump:\n";
    cout << "Resource:      " << resource_name << "\n";
    cout << "Sema_value:    " << sema_value << "\n";
    cout << "Sema_queue:    ";

    if (sema_queue->isEmpty()) {
        cout << "(empty)";
    } else {
        cout << sema_queue->Get_Q_String();
    }

    cout << "\n\n";

    pthread_mutex_unlock(&lock);
}