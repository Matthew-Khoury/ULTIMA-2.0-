#ifndef MCB_H
#define MCB_H

#include "Scheduler.h"
#include "IPC.h"
#include "Semaphore.h"
#include <string>

class MCB {
public:
	scheduler Swapper;  // scheduler
	ipc Messenger;      // IPC
	Semaphore Monitor;  // example semaphore for a monitor
	Semaphore Printer;  // example semaphore for a printer

	// Constructor
	MCB();

	// Initialize IPC after scheduler tasks exist
	int InitIPC();
};

#endif