// Created by Matthew Khoury

#include "MCB.h"

// Constructor
MCB::MCB()
	: Monitor(1, "Monitor", &Swapper),   // assign scheduler pointer
	  Printer(1, "Printer", &Swapper)    // assign scheduler pointer
{
	// At this point, scheduler is constructed, tasks may not exist yet
}

// Initialize IPC
int MCB::InitIPC() {
	// Initialize IPC with the number of tasks from scheduler
	int result = Messenger.Init(MAX_TASKS, &Swapper);

	// Assign mailbox semaphores to each task (already done in Messenger.Init)
	return result;
}