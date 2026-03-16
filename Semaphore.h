#include <pthread.h>
#include "Queue.h"


class Semaphore {
private:
	string resource_name;     // Optional: name of the protected resource
	int sema_value;           // Current semaphore count (can be >1)
	int lucky_task;           // preserve the taskID of the task that has the resource

	Queue<int> *sema_queue;   // Queue of waiting thread IDs (FIFO)
	scheduler *sched_ptr;

	pthread_mutex_t lock{};       // Protects access to sema_value and queue
	pthread_cond_t cond{};        // Used to block and wake waiting threads

public:
	Semaphore(int initialValue, string name, scheduler *theScheduler); // Semaphore constructor
	~Semaphore();               // Semaphore deconstructor

	void down(int taskID);
	void up();
	void dump();
};
