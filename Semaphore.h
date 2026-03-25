/* Code Framework built from Lab 10
Primary Author: Matthew Khoury*/

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <string>
#include <pthread.h>
#include "Queue.h"
#include "Scheduler.h"

using std::string;

class Semaphore {
private:
	string resource_name;     // Optional: name of the protected resource
	int sema_value;           // Current semaphore count (can be >1)
	int lucky_task;           // Task ID that currently owns the resource

	Queue<int>* sema_queue;   // Queue of waiting task IDs (FIFO)
	scheduler* sched_ptr;     // Pointer to the scheduler

	pthread_mutex_t lock;     // Protects access to sema_value and queue
	pthread_cond_t cond;      // Used to block and wake waiting tasks

public:
	Semaphore(int initialValue, string name, scheduler* theScheduler);
	~Semaphore();

	void down(int taskID);
	void up();
	void dump();

	string get_resource_name();
	int get_sema_value();
	int get_lucky_task();
	string get_queue_string();
};

#endif