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

// down(): attempt to acquire the resource
void Semaphore::down(int taskID) {
    pthread_mutex_lock(&lock);

    if (taskID == lucky_task) {
        cout << "Task #" << lucky_task
             << " already has the resource! Ignoring request." << endl;
        dump();
    } else {
        if (sema_value > 0) {
            // Resource available
            sema_value--;
            lucky_task = taskID;
            dump();
        } else {
            // No resource → block
            sema_queue->En_Q(taskID);
            sched_ptr->set_state(taskID, BLOCKED);
            dump();

            // Wait until someone does up() and signals
            while (sema_value == 0) {
                sched_ptr->yield();              // let scheduler run others
                dump();
                pthread_cond_wait(&cond, &lock); // sleep until signaled
            }

            // We were woken up and resource is now available
            sema_value--;
            lucky_task = taskID;
            cout << "\tTask " << taskID
                 << " has been released and acquired the resource" << endl;
            dump();
        }
    }

    pthread_mutex_unlock(&lock);
}

// up(): release the resource
void Semaphore::up() {
    pthread_mutex_lock(&lock);

    int current_id = sched_ptr->get_task_id();
    cout << "TaskID: " << current_id
         << " LuckyID: " << lucky_task << endl;

    if (current_id == lucky_task) {   // only owner can release
        if (sema_queue->isEmpty()) {
            // No one waiting → just increment
            sema_value++;
            lucky_task = -1;
            dump();
        } else {
            // Someone is waiting → wake one
            int taskID = sema_queue->De_Q();
            sched_ptr->set_state(taskID, READY);
            cout << "Unblock task " << taskID
                 << " and release from the queue" << endl;

            // Make resource available for the woken task
            sema_value = 1;
            lucky_task = -1;

            pthread_cond_signal(&cond);  // wake one waiter
            dump();

            // Let scheduler pick the next READY task
            sched_ptr->yield();
            dump();
        }
    } else {
        cout << "Invalid Semaphore::up(). TaskID " << current_id
             << " does not own the resource." << endl;
        dump();
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
