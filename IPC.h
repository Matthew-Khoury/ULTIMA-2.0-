// Created by Matthew Khoury

#ifndef IPC_H
#define IPC_H

#include <ctime>

#define MAX_MSG_SIZE 32


class scheduler;  // forward declaration

class ipc {
public:
	struct Message_Type {
		int Message_Type_Id;
		char Message_Type_Description[64];
	};

	struct Message {
		int Source_Task_Id;
		int Destination_Task_Id;
		time_t Message_Arrival_Time;
		Message_Type Msg_Type;       // research time.h, time_t, and tm
		int Msg_Size;
		char Msg_Text[MAX_MSG_SIZE]; // let's make this up to 32 bytes long
	};

private:
	int max_tasks;
	scheduler* sched_ptr;  // for tcb access

public:
	// Default constructor
	ipc() : max_tasks(0), sched_ptr(nullptr) {}
	// Deconstructor
	~ipc();

	// Constructor, return -1 if error
	int Init(int max_tasks, scheduler* scheduler); // initialize mailboxes

	int Message_Send(Message *msg); // returns -1 if error occurred. Return 1 if successful

	// Overloaded send
	int Message_Send(int source_id, int dest_id, char* text, int type);

	int Message_Receive(int task_id, Message *msg);// returns 0 if no more messages are available, loads the Message
	// structure with the first message from the mailbox and remove the
	// message from the mailbox. Return -1 if an error occurs.

	// Overloaded receive
	int Message_Receive(int task_id, char* text, int* type);

	// Message counts
	int Message_Count(int task_id); // return the number of messages in Task-id’s  message queue
	int Message_Count();           // return the total number of messages in all the message queues. (overloaded method)

	// Debug / utility
	void Message_Print(int task_id);    // print all messages for a given Task-id. (but do not remove from the queue)
	int Message_DeleteAll(int task_id); // delete all the messages for Task_id, from the queue.

	void ipc_Message_Dump();			// print all the messages in the message queue, but do not delete them from the queue.
	// (print the messages for each task’s message queue)
};

#endif