#include "Sema.h"
#include <iostream>
#include <unistd.h>

// Constructor - Initializes semaphore value, mutex, condition variable, and queue
Semaphore::Semaphore(int initialValue, string name, scheduler *theScheduler) {
    sema_value = initialValue;
    resource_name = name;
    lucky_task = -1;

    pthread_mutex_init(&lock, nullptr);
    pthread_cond_init(&cond, nullptr);

    // Queue stores thread IDs waiting for the semaphore
    sema_queue = new Queue<int>();
    
    // register the scheduler with the semaphore
    sched_ptr = theScheduler;
}

// Destructor - cleans up queue and pthread sync objects
Semaphore::~Semaphore() {
    delete sema_queue;

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
}

// down(): attempts to obtain a semaphore
// -  if taskID == lucky_task: skip operation
// -  if sema_value >= 1: decrement and continue
// -  if sema_value == 0: block the thread and put in the queue
// Uses pthread_cond_wait() to avoid busy waiting
void Semaphore::down(int taskID) {
    pthread_mutex_lock(&lock);

    // Enter critical section
    if (taskID == lucky_task) {
        cout << "Task # " << lucky_task << " already has the resource!  Ignore the request" << std::endl;
        dump();
    } else {
        if (sema_value >= 1) {
            // Resource available → decrement
            sema_value--;
            lucky_task = taskID;  // preserve the taskID that got the resource
            dump();
        }
        else {
            sema_queue->En_Q(taskID);            // Add thread ID to Queue
            sched_ptr->set_state(taskID, BLOCKED);  // No resource available → block
            dump();

            // Block until signaled by up()
            // pthread_cond_wait() releases the mutex while waiting and re-acquires it before returning
            do {
                sched_ptr->yield();
                dump();
                pthread_cond_wait(&cond, &lock);
            } while (sema_value < 0);

            std::cout << "\tThread " << taskID
                      << " has been released and re-acquired the mutex" << std::endl;
        }
    }
    // Exit critical section
    pthread_mutex_unlock(&lock);
}

// up(): releases the semaphore
//   - if threads are waiting → dequeue one and signal it
//   - else → increment sema_value
void Semaphore::up() {
    int taskID;

    pthread_mutex_lock(&lock);

    cout << "TaskID: " << sched_ptr.get_task_id() << " LuckyID: " << lucky_task << endl;
    if (sched_ptr.get_task_id() == lucky_task) {   // check to see if the correct task is doing the up()
        if (sema_queue->isEmpty()) {
            sema_value++;
            lucky_task = -1;
            dump();
        } else {
            taskID = sema_queue->De_Q();         // remove from Queue and unblock
            sched_ptr->set_state(taskID, READY); // set the task to READY
            cout << "Unblock: " << taskID << " and release from the queue" << endl;
            dump();
            sched_ptr->yield();
            dump();
        }
    } else {
        cout << "Invalid Semaphore UP().  TaskID: " << sched_ptr.get_task_id() <<
                " does not own the resource" << endl;
        dump();
    }

    // Exit critical section
    pthread_mutex_unlock(&lock);
}

void Semaphore::dump() {
    pthread_mutex_lock(&lock);

    std::cout << "\nSemaphore Dump:\n";
    std::cout << "Resource:      " << resource_name << "\n";
    std::cout << "Sema_value:    " << sema_value << "\n";
    std::cout << "Sema_queue:    ";

    if (sema_queue->isEmpty()) {
        std::cout << "(empty)";
    } else {
        // Print queue contents in FIFO order
        std::cout << sema_queue->Get_Q_String();
    }

    std::cout << "\n\n";

    pthread_mutex_unlock(&lock);
}
