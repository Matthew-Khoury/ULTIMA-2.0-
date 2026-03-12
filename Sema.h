#include <pthread.h>
#include "Queue.h"


class Semaphore {
private:
	char resource_name[64];     // Optional: name of the protected resource
	int sema_value;             // Current semaphore count (can be >1)
	Queue<int>* sema_queue;     // Queue of waiting thread IDs (FIFO)

	pthread_mutex_t lock;       // Protects access to sema_value and queue
	pthread_cond_t cond;        // Used to block and wake waiting threads

public:
	Semaphore(int initialValue);// Semaphore constructor

	~Semaphore();               // Semaphore deconstructor

	void down(int thread_id);
	void up(int thread_id);
};
