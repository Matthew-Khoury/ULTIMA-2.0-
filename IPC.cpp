#include "IPC.h"
#include <cstring>
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

    tcb* dest_task = sched_ptr->get_task(msg->Destination_Task_Id);

    // Only check mailbox_sema
    if (dest_task == nullptr || !dest_task->mailbox_sema) return -1;

    dest_task->mailbox_sema->down(msg->Source_Task_Id);
    dest_task->mailbox.En_Q((int)(intptr_t)msg);
    dest_task->mailbox_sema->up();

    return 1;
}

// Receive a message for this task
int ipc::Message_Receive(int task_id, Message* msg) {
    tcb* task = sched_ptr->get_task(task_id);
    if (task == nullptr || !task->mailbox_sema) return -1;

    task->mailbox_sema->down(task_id);

    if (task->mailbox.isEmpty()) {
        task->mailbox_sema->up();
        return 0;
    }

    // Get the pointer from the queue
    Message* queued_msg = (Message*)(intptr_t)task->mailbox.De_Q();

    // Copy the data into the msg pointer provided by the caller
    if (queued_msg != nullptr && msg != nullptr) {
        memcpy(msg, queued_msg, sizeof(Message));

        // TODO: Finish implementing the receive logic and test
        // If the sender used 'new' to create this, delete it now
        // delete queued_msg;
    }

    task->mailbox_sema->up();
    return 1;
}

// Return the number of messages in Task-id’s message queue
int ipc::Message_Count(int task_id) {

    if (task_id < 0 || task_id >= MAX_TASKS) {
        return -1;
    }

    tcb* task = sched_ptr->get_task(task_id);

    if (task == nullptr) {
        return 0;
    }

    return task->mailbox.Size();
}

// Destructor
ipc::~ipc() {
    for (int i = 0; i < max_tasks; i++) {
        tcb* task = sched_ptr->get_task(i);
        if (task && task->mailbox_sema) {
            delete task->mailbox_sema;
            task->mailbox_sema = nullptr;
        }
    }
}