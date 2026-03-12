#include "Sema.h"
#include <iostream>
#include <unistd.h>

// Constructor - Initializes semaphore value, mutex, condition variable, and queue
Semaphore::Semaphore(int initialValue) {
    sema_value = initialValue;

    pthread_mutex_init(&lock, nullptr);
    pthread_cond_init(&cond, nullptr);

    // Queue stores thread IDs waiting for the semaphore
    sema_queue = new Queue<int>();
}

// Destructor - cleans up queue and pthread sync objects
Semaphore::~Semaphore() {
    delete sema_queue;

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
}

// down(): attempts to obtain a semaphore
// -  if sema_value >= 1: decrement and continue
// -  if sema_value == 0: block the thread and put in the queue
// Uses pthread_cond_wait() to avoid busy waiting
void Semaphore::down(int thread_id) {
    pthread_mutex_lock(&lock);

    // Enter critical section

    if (sema_value >= 1) {
        // Resource available → decrement
        sema_value--;
    }
    else {
        // No resource available → block
        pthread_t self = pthread_self();

        std::cout << "\tThread " << thread_id << " is being placed on the wait queue (pthread id = "
                  << self << ")" << std::endl;

        // Add thread ID to Queue
        sema_queue->En_Q(thread_id);
        sema_queue->Print();

        // Block until signaled by up()
        // pthread_cond_wait() releases the mutex while waiting and re-acquires it before returning
        do {
            std::cout << "\tThread " << thread_id
                      << " is blocked and waiting..." << std::endl;

            pthread_cond_wait(&cond, &lock);

        } while (sema_value < 0);

        std::cout << "\tThread " << thread_id
                  << " has been released and re-acquired the mutex" << std::endl;
    }

    // Exit critical section
    pthread_mutex_unlock(&lock);
}

// up(): releases the semaphore
//   - if threads are waiting → dequeue one and signal it
//   - else → increment sema_value
void Semaphore::up(int thread_id) {
    pthread_mutex_lock(&lock);

    // Enter critical section
    if (sema_value <= 0) {
        // Threads are waiting → wake one
        int id = -1;

        if (!sema_queue->isEmpty()) {
            std::cout << "\tBefore releasing a thread from the queue:\n";
            sema_queue->Print();

            // Remove next waiting thread (FIFO)
            id = sema_queue->De_Q();

            std::cout << "\tReleasing thread " << id << " from queue\n";
            sema_queue->Print();
        }

        std::cout << "\tSignaling blocked thread " << id << std::endl;

        // wake one blocked thread
        pthread_cond_signal(&cond);
    }
    else {
        // No threads waiting → increment
        sema_value++;
    }

    // Exit critical section
    pthread_mutex_unlock(&lock);
}
