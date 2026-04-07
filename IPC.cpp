// Primary Author: Matthew Khoury
// Co-contributor Dylan Hurt

#include "IPC.h"
#include <cstring>
#include <cstdint>
#include <ctime>
#include <iostream>
#include "Scheduler.h"
#include "Semaphore.h"

using namespace std;

int ipc::Init(int max_tasks_, scheduler* scheduler) {
    if (!scheduler) return -1;
    sched_ptr = scheduler;
    max_tasks = max_tasks_;

    // Initialize mailbox semaphores
    for (int i = 0; i < max_tasks; i++) {

        tcb* task = sched_ptr->get_task(i);  // always valid for 0 <= i < MAX_TASKS

        if (task == nullptr) continue;

        // Only check mailbox_sema, no need to check task pointer
        task->mailbox_sema = new Semaphore(1, "Mailbox", sched_ptr);

        while (!task->mailbox.isEmpty()) {
            task->mailbox.De_Q();
        }
    }

    return 1;
}

// Send a message to destination task
int ipc::Message_Send(Message* msg) {
    if (!msg) return -1;
    if (sched_ptr == nullptr) return -1;
    if(sched_ptr->get_state(msg->Destination_Task_Id) != DEAD) return -1;


    tcb* dest_task = sched_ptr->get_task(msg->Destination_Task_Id);

    // Only check mailbox_sema
    if (dest_task == nullptr || !dest_task->mailbox_sema || dest_task->state == DEAD) return -1;

    // Fill in arrival time
    if (msg->Message_Arrival_Time == 0)
        msg->Message_Arrival_Time = time(nullptr);
        

    // Fill in message type description
    if (msg->Msg_Type.Message_Type_Id == 0) {
        strcpy(msg->Msg_Type.Message_Type_Description, "Text");
    }
    else if (msg->Msg_Type.Message_Type_Id == 1) {
        strcpy(msg->Msg_Type.Message_Type_Description, "Service");
    }
    else if (msg->Msg_Type.Message_Type_Id == 2) {
        strcpy(msg->Msg_Type.Message_Type_Description, "Notification");
    }
    else {
        strcpy(msg->Msg_Type.Message_Type_Description, "Unknown");
    }

    msg->Msg_Text[MAX_MSG_SIZE - 1] = '\0';
    msg->Msg_Size = strlen(msg->Msg_Text);

    // Enqueue the message
    try {
        dest_task->mailbox.En_Q((intptr_t)msg);
    }
    catch (const std::runtime_error&) {
        return -1;
    }

    return 1;
}

// Receive a message for this task
int ipc::Message_Receive(int task_id, Message* msg) {
    if (msg == nullptr) return -1;
    if (sched_ptr == nullptr) return -1;

    tcb* task = sched_ptr->get_task(task_id);
    if (task == nullptr || !task->mailbox_sema) return -1;
    if (task->mailbox.isEmpty()) {
        return 0;
    }

    // Get the message pointer from the queue
    Message* queued_msg = (Message*)task->mailbox.De_Q();

    // Copy the data into the message pointer provided by the caller
    if (queued_msg != nullptr) {
        memcpy(msg, queued_msg, sizeof(Message));

        msg->Msg_Text[MAX_MSG_SIZE - 1] = '\0';
        msg->Msg_Size = strlen(msg->Msg_Text);

        if (msg->Msg_Type.Message_Type_Id == 0) {
            strcpy(msg->Msg_Type.Message_Type_Description, "Text");
        }
        else if (msg->Msg_Type.Message_Type_Id == 1) {
            strcpy(msg->Msg_Type.Message_Type_Description, "Service");
        }
        else if (msg->Msg_Type.Message_Type_Id == 2) {
            strcpy(msg->Msg_Type.Message_Type_Description, "Notification");
        }
        else {
            strcpy(msg->Msg_Type.Message_Type_Description, "Unknown");
        }

        delete queued_msg;
        return 1;
    }

    return -1;
}

// Return the number of messages in task_id's message queue
int ipc::Message_Count(int task_id) {
    if (sched_ptr == nullptr) return -1;

    if (task_id < 0 || task_id >= MAX_TASKS) {
        return -1;
    }

    tcb* task = sched_ptr->get_task(task_id);

    if (task == nullptr || !task->mailbox_sema) {
        return 0;
    }

    return task->mailbox.Size();
}

// Displays all messages in the specified task's mailbox
void ipc::Message_Print(int task_id) {
    if (sched_ptr == nullptr) return;
    if (task_id < 0 || task_id >= MAX_TASKS) return;

    tcb* task = sched_ptr->get_task(task_id);
    if (task == nullptr) return;

    for (int i = 0; i < task->mailbox.Size(); i++) {
        Message* msg = (Message*)(intptr_t)task->mailbox.Peek(i);
        if (msg != nullptr) {
            cout << "Src=" << msg->Source_Task_Id
                 << " Dst=" << msg->Destination_Task_Id
                 << " Text=" << msg->Msg_Text
                 << " Size=" << msg->Msg_Size
                 << " Type=" << msg->Msg_Type.Message_Type_Description
                 << endl;
        }
    }
}

// Deletes all messages in the specified task's mailbox
int ipc::Message_DeleteAll(int task_id) {
    if (task_id < 0 || task_id >= MAX_TASKS) return -1;

    tcb* task = sched_ptr->get_task(task_id);
    if (task == nullptr) return -1;

    int deleted = 0;

    while (!task->mailbox.isEmpty()) {
        Message* queued_msg = (Message*)task->mailbox.De_Q();
        if (queued_msg != nullptr) {
            delete queued_msg;
        }
        deleted++;
    }

    return deleted;
}

// Message Dump
void ipc::ipc_Message_Dump() {
    if (sched_ptr == nullptr) return;

    for (int i = 0; i < max_tasks; i++) {
        cout << "Mailbox for Task " << i << ":" << endl;
        Message_Print(i);
    }
}

// Destructor
ipc::~ipc() {
    for (int i = 0; i < max_tasks; i++) {
        Message_DeleteAll(i);

        tcb* task = sched_ptr->get_task(i);
        if (task && task->mailbox_sema) {
            delete task->mailbox_sema;
            task->mailbox_sema = nullptr;
        }
    }
}