// Primary Author: Matthew Khoury
// Co-contributor Dylan Hurt & Melanie Priestly 

#include "IPC.h"
#include "Scheduler.h"
#include "Semaphore.h"
#include "CryptoUnit.h"
#include <cstring>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <vector>
#include <openssl/rand.h>
#include <openssl/evp.h>

using namespace std;

int ipc::Init(int max_tasks_, scheduler* scheduler)
{
    if (!scheduler) return -1;

    sched_ptr = scheduler;
    max_tasks = max_tasks_;

    for (int i = 0; i < max_tasks; i++)
    {
        tcb* task = sched_ptr->get_task(i);
        if (!task) continue;

        task->mailbox_sema = new Semaphore(1, "Mailbox", sched_ptr);

        while (!task->mailbox.isEmpty())
            task->mailbox.De_Q();
    }

    return 1;
}

// send message using hybrid RSA + AES
int ipc::Message_Send(Message* msg){
    if (!msg || !sched_ptr) return -1;

    tcb* dest = sched_ptr->get_task(msg->Destination_Task_Id);
    if (!dest || dest->state == DEAD) return -1;

    msg->Message_Arrival_Time = time(nullptr);

    // 1. generate keys
    std::vector<unsigned char> aes_key(32);
    std::vector<unsigned char> iv(16);

    RAND_bytes(aes_key.data(), 32);
    RAND_bytes(iv.data(), 16);

    // 2. encrypt plaintext
    std::vector<unsigned char> plain(
        msg->plaintext.begin(),
        msg->plaintext.end()
    );

    msg->packet.cipher =
        CryptoUnit::aesEncrypt(plain, aes_key, iv);

    msg->packet.iv = iv;

    msg->packet.rsa_enc_key =
        CryptoUnit::rsaEncryptKey(aes_key);

    // 3. enqueue REAL POINTER (no casts)
    dest->mailbox.En_Q(msg);

    return 1;
}

// receive message using hybrid decryption
int ipc::Message_Receive(int task_id, Message* out)
{
    if (!out || !sched_ptr) return -1;

    tcb* task = sched_ptr->get_task(task_id);
    if (!task || task->mailbox.isEmpty()) return 0;

    Message* msg = task->mailbox.De_Q();
    if (!msg) return -1;

    *out = *msg; // safe struct copy

    // decrypt
    std::vector<unsigned char> aes_key =
        CryptoUnit::rsaDecryptKey(msg->packet.rsa_enc_key);

    std::vector<unsigned char> plain =
        CryptoUnit::aesDecrypt(
            msg->packet.cipher,
            aes_key,
            msg->packet.iv
        );

    out->plaintext.assign(
        plain.begin(),
        plain.end()
    );

    delete msg;
    return 1;
}

// Return the number of messages in task_id's message queue
int ipc::Message_Count(int task_id)
{
    if (!sched_ptr) return -1;
    if (task_id < 0 || task_id >= max_tasks) return -1;

    tcb* task = sched_ptr->get_task(task_id);
    if (!task) return 0;

    return task->mailbox.Size();
}

// Return the number of messages (global)
int ipc::Message_Count()
{
    if (!sched_ptr) return -1;

    int total = 0;

    for (int i = 0; i < max_tasks; i++)
    {
        tcb* task = sched_ptr->get_task(i);
        if (task)
            total += task->mailbox.Size();
    }

    return total;
}

// Deletes all messages in the specified task's mailbox
int ipc::Message_DeleteAll(int task_id)
{
    if (!sched_ptr) return -1;
    if (task_id < 0 || task_id >= max_tasks) return -1;

    tcb* task = sched_ptr->get_task(task_id);
    if (!task) return -1;

    int deleted = 0;

    while (!task->mailbox.isEmpty())
    {
        Message* msg = (Message*)task->mailbox.De_Q();
        if (msg)
        {
            delete msg;
            deleted++;
        }
    }

    return deleted;
}

int ipc::Message_Receive(int task_id, char* text, int* type)
{
    if (!text || !type) return -1;

    Message msg;

    int result = Message_Receive(task_id, &msg);
    if (result != 1) return result;

    strncpy(text, msg.plaintext.c_str(), MAX_MSG_SIZE - 1);
    text[MAX_MSG_SIZE - 1] = '\0';

    *type = msg.Msg_Type.Message_Type_Id;

    return 1;
}

void ipc::Message_Print(int task_id)
{
    if (!sched_ptr) return;

    tcb* task = sched_ptr->get_task(task_id);
    if (!task) return;

    for (int i = 0; i < task->mailbox.Size(); i++)
    {
        Message* msg = (Message*)(intptr_t)task->mailbox.Peek(i);
        if (!msg) continue;

        cout
            << "[SRC " << msg->Source_Task_Id << "] "
            << "[DST " << msg->Destination_Task_Id << "] "
            << msg->plaintext
            << " (" << msg->Msg_Type.Message_Type_Description << ")"
            << endl;
    }
}

// Message Dump
void ipc::ipc_Message_Dump()
{
    if (!sched_ptr) return;

    for (int i = 0; i < max_tasks; i++)
    {
        cout << "Mailbox Task " << i << ":\n";
        Message_Print(i);
    }
}

// Destructor
ipc::~ipc()
{
    if (!sched_ptr) return;

    for (int i = 0; i < max_tasks; i++)
    {
        Message_DeleteAll(i);

        tcb* task = sched_ptr->get_task(i);
        if (task && task->mailbox_sema)
        {
            delete task->mailbox_sema;
            task->mailbox_sema = nullptr;
        }
    }
}
