/* Code Framework built from Lab 6
Primary Author: Dylan Hurt
*/
#include "Ultima.h"
#include "CryptoUnit.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <vector>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <iostream>

static std::string hex_preview(const std::vector<unsigned char>& bytes, size_t max_bytes = 16)
{
    std::string preview;

    for (size_t i = 0; i < bytes.size() && i < max_bytes; i++)
    {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X ", bytes[i]);
        preview += buf;
    }

    if (preview.empty())
        return "(empty)";

    if (bytes.size() > max_bytes)
        preview += "...";

    return preview;
}

// ULTIMA Constructor
ULTIMA::ULTIMA()
    : heading_win_(nullptr),
      task_win_(nullptr),
      sema_win_(nullptr),
      mailbox_win_(nullptr),
      memory_usage_win_(nullptr),
      core_dump_win_(nullptr),
      log_win_(nullptr),
      console_win_(nullptr),
      mcb_(),
      running_(true),
      paused_(false),
      log_line_(1),
      selected_mailbox_(0),
      selected_semaphore_(0),
      show_core_dump_(true)
{
}

// Destructor
ULTIMA::~ULTIMA()
{
    shutdown_curses();
}

// init_curses()
void ULTIMA::init_curses()
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    refresh();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int margin = 1;
    int full_width = cols - 2;
    int half_width = (full_width - 1) / 2;

	const int heading_height = 6;
	const int top_panel_start = heading_height;

	heading_win_ = create_window(heading_height, full_width, 0, margin);

	task_win_ = create_window(9, half_width, top_panel_start, margin);
	sema_win_ = create_window(9, full_width - half_width - 1, top_panel_start, margin + half_width + 1);

	const int mailbox_height = 20;
	const int memory_height = 10;
	const int mailbox_start = top_panel_start + 9;
	const int memory_start = mailbox_start + mailbox_height;

	mailbox_win_ = create_window(mailbox_height, full_width, mailbox_start, margin);
	memory_usage_win_ = create_window(memory_height, full_width, memory_start, margin);

    int bottom_start = memory_start + memory_height;
    int bottom_height = rows - bottom_start - 1;

    if (bottom_height < 10)
        bottom_height = 10;

    int log_width = (full_width * 2) / 3;
    int console_width = full_width - log_width - 1;

    core_dump_win_ = create_window(bottom_height / 2, full_width, bottom_start, margin);
    log_win_ = create_window(bottom_height - (bottom_height / 2), log_width,
                             bottom_start + (bottom_height / 2), margin);
    console_win_ = create_window(bottom_height - (bottom_height / 2), console_width,
                                 bottom_start + (bottom_height / 2), margin + log_width + 1);

    draw_log("----- ULTIMA Log Started -----");
}


// shutdown_curses()
void ULTIMA::shutdown_curses()
{
    if (stdscr != nullptr)
        endwin();
}

// create_window()
WINDOW* ULTIMA::create_window(int height, int width, int y, int x)
{
    WINDOW* win = newwin(height, width, y, x);
    box(win, 0, 0);
    wrefresh(win);
    return win;
}

// initialize_scheduler()
void ULTIMA::initialize_scheduler()
{
    CryptoUnit::initialize();

    draw_log("Scheduler started.");

    for (int i = 0; i < MAX_TASKS; i++)
        mcb_.Swapper.create_task();

    mcb_.Swapper.start();
    mcb_.InitIPC();

    draw_log("IPC initialized.");
    draw_log("Memory manager initialized.");
}

// draw_heading()
void ULTIMA::draw_heading()
{
    werase(heading_win_);
    box(heading_win_, 0, 0);

    int current_task_id = mcb_.Swapper.get_task_id();
    long quantum = mcb_.Swapper.get_quantum();
    clock_t elapsed = 0;
    long remaining = quantum;

    if (current_task_id >= 0)
    {
        elapsed = mcb_.Swapper.get_elapsed_time(current_task_id);
        remaining = quantum - (long)elapsed;
        if (remaining < 0) remaining = 0;
    }

    mvwprintw(heading_win_, 1, 2, "ULTIMA Project");
    mvwprintw(heading_win_, 2, 2, "Current Task ID: %d", current_task_id);
    mvwprintw(heading_win_, 2, 34, "Quantum: %ld  Elapsed: %ld  Remaining: %ld",
              quantum, (long)elapsed, remaining);
    mvwprintw(heading_win_, 3, 2, "Mem Left: %d  Largest Free: %d  Smallest Free: %d  Active Handles: %d",
              mcb_.MemMgr.Mem_Left(),
              mcb_.MemMgr.Mem_Largest(),
              mcb_.MemMgr.Mem_Smallest(),
              active_memory_handle_count());
    mvwprintw(heading_win_, 4, 2, "Crypto: AES-256-CTR (MMU) | RSA-2048 Hybrid (IPC)");

    wrefresh(heading_win_);
}

// draw_tasks()
void ULTIMA::draw_tasks()
{
    werase(task_win_);
    box(task_win_, 0, 0);

    mvwprintw(task_win_, 1, 2, "Process Table Dump:");
    mvwprintw(task_win_, 2, 2, "%-12s %-8s %-10s %-12s", "TaskName", "TaskID", "State", "MemHandle");
    mvwprintw(task_win_, 3, 2, "----------------------------------------------------------");

    int line = 4;
    for (int i = 0; i < mcb_.Swapper.get_task_count(); i++)
    {
        tcb* node = mcb_.Swapper.get_current();
        bool in_list = false;
        if (node)
        {
            tcb* start = node;
            do
            {
                if (node->task_id == i)
                {
                    in_list = true;
                    break;
                }
                node = node->next;
            } while (node != start);
        }

        if (!in_list) continue;
        if (line >= 8) break;

        std::string task_name = "Task " + std::to_string(i);
        char handle_text[32];
        const tcb* task = mcb_.Swapper.get_task(i);

        if (task && !task->memory_regions.empty() &&
            task->active_region_index >= 0 &&
            task->active_region_index < (int)task->memory_regions.size())
        {
            int handle = task->memory_regions[task->active_region_index].handle;
            snprintf(handle_text, sizeof(handle_text), "%d (%d)", handle, (int)task->memory_regions.size());
        }
        else
        {
            snprintf(handle_text, sizeof(handle_text), "None");
        }

        mvwprintw(task_win_, line, 2, "%-12s %-8d %-10s %-12s",
                  task_name.c_str(), i, mcb_.Swapper.get_state(i).c_str(), handle_text);
        line++;
    }
    wrefresh(task_win_);
}

// draw_semaphore()
void ULTIMA::draw_semaphore()
{
    werase(sema_win_);
    box(sema_win_, 0, 0);

    Semaphore* current_sema = (selected_semaphore_ == 0) ? &mcb_.Monitor : &mcb_.Printer;

    mvwprintw(sema_win_, 1, 2, "Semaphore Dump:");
    mvwprintw(sema_win_, 2, 2, "Resource: %s", current_sema->get_resource_name().c_str());
    mvwprintw(sema_win_, 3, 2, "Sema_value: %d", current_sema->get_sema_value());
    mvwprintw(sema_win_, 4, 2, "Sema_queue: %s", current_sema->get_queue_string().c_str());
    mvwprintw(sema_win_, 5, 2, "Lucky_task: %d", current_sema->get_lucky_task());
    mvwprintw(sema_win_, 7, 2, "t : toggle resource / d : down / u : up");

    wrefresh(sema_win_);
}

// draw_mailboxes()
void ULTIMA::draw_mailboxes()
{
    werase(mailbox_win_);
    box(mailbox_win_, 0, 0);

    int task_id = selected_mailbox_;
    bool in_list = false;
    tcb* node = mcb_.Swapper.get_current();
    if (node)
    {
        tcb* start = node;
        do
        {
            if (node->task_id == task_id)
            {
                in_list = true;
                break;
            }
            node = node->next;
        } while (node != start);
    }

    mvwprintw(mailbox_win_, 1, 2, "Mailbox Dump:");
    mvwprintw(mailbox_win_, 2, 2, "Task Number: %d", task_id);

    if (!in_list)
    {
        mvwprintw(mailbox_win_, 3, 2, "Message Count: 0");
        mvwprintw(mailbox_win_, 5, 2, "(mailbox no longer exists)");
        mvwprintw(mailbox_win_, 18, 2, "m : switch / s : text / v : service / n : notification");
        wrefresh(mailbox_win_);
        return;
    }

    int message_count = mcb_.Messenger.Message_Count(task_id);
    if (message_count < 0) message_count = 0;

    mvwprintw(mailbox_win_, 3, 2, "Message Count: %d", message_count);
    mvwprintw(mailbox_win_, 5, 2, "Src  Dst  Encrypted (Hex Preview)");
    mvwprintw(mailbox_win_, 6, 2, "          Decrypted (Plaintext)");
    mvwprintw(mailbox_win_, 7, 2, "          Enc/Dec Sizes + Type + Time");
    if (message_count == 0)
    {
        mvwprintw(mailbox_win_, 9, 2, "(empty)");
    }
    else
    {
        tcb* task = mcb_.Swapper.get_task(task_id);
        if (task != nullptr)
        {
            int row = 9;

            int max_y, max_x;
            getmaxyx(mailbox_win_, max_y, max_x);
            int message_width = max_x - 22;
            if (message_width < 20)
                message_width = 20;

            for (int i = 0; i < task->mailbox.Size() && row < max_y - 4; i++)
            {
                ipc::Message* msg = (ipc::Message*)(intptr_t)task->mailbox.Peek(i);

                char time_buffer[16] = "N/A";

                if (msg->Message_Arrival_Time != 0)
                {
                    struct tm* tm_info = localtime(&msg->Message_Arrival_Time);
                    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", tm_info);
                }

                std::string encrypted_preview = hex_preview(msg->packet.cipher);

                // Show encrypted size only (NO raw parsing anymore)
                int encrypted_size = (int)msg->packet.cipher.size();

                std::string decrypted_text = msg->plaintext;
                if (decrypted_text.empty())
                    decrypted_text = "(not decrypted yet)";

                char type_char = 'U';

                switch (msg->Msg_Type.Message_Type_Id)
                {
                    case 0: type_char = 'T'; break;
                    case 1: type_char = 'S'; break;
                    case 2: type_char = 'N'; break;
                }

                mvwprintw(mailbox_win_, row, 2,
                    "%-4d %-4d Encrypted(%dB): %s",
                    msg->Source_Task_Id,
                    msg->Destination_Task_Id,
                    encrypted_size,
                    encrypted_preview.c_str()
                );

                row++;

                if (row >= max_y - 2)
                    break;

                mvwprintw(mailbox_win_, row, 12,
                    "Decrypted: %s",
                    decrypted_text.c_str()
                );

                row++;

                if (row >= max_y - 2)
                    break;

                mvwprintw(mailbox_win_, row, 12,
                    "[%c] Type=%s Time=%s",
                    type_char,
                    msg->Msg_Type.Message_Type_Description.c_str(),
                    time_buffer
                );

                row++;
            }
        }
    }

    mvwprintw(mailbox_win_, 18, 2, "m : switch / s : text / v : service / n : notification");
    wrefresh(mailbox_win_);
}

// helper
int ULTIMA::find_task_for_handle(int handle) const
{
    if (handle <= 0) return -1;

    for (int i = 0; i < MAX_TASKS; i++)
    {
        const tcb* task = mcb_.Swapper.get_task(i);
        if (!task) continue;

        for (int j = 0; j < (int)task->memory_regions.size(); j++)
        {
            if (task->memory_regions[j].handle == handle)
                return i;
        }
    }
    return -1;
}

// helper
int ULTIMA::active_memory_handle_count() const
{
    int count = 0;
    for (int i = 0; i < MAX_TASKS; i++)
    {
        const tcb* task = mcb_.Swapper.get_task(i);
        if (!task) continue;

        count += task->memory_regions.size();
    }
    return count;
}

// helper
void ULTIMA::free_task_memory(tcb* task)
{
    if (task == nullptr)
        return;

    for (int i = 0; i < (int)task->memory_regions.size(); i++)
        mcb_.MemMgr.Mem_Free(task->memory_regions[i].handle);

    task->memory_regions.clear();
    task->active_region_index = -1;
}

// draw_memory_usage()
void ULTIMA::draw_memory_usage()
{
    werase(memory_usage_win_);
    box(memory_usage_win_, 0, 0);

    mvwprintw(memory_usage_win_, 1, 2, "Memory Usage:");
    mvwprintw(memory_usage_win_, 2, 2,
              "%-8s %-13s %-10s %-10s %-11s %-16s %-8s",
              "Status", "MemHandle", "Start", "End", "Size", "CurrentLoc", "Task-ID");
    mvwprintw(memory_usage_win_, 3, 2,
              "------------------------------------------------------------------------------------------------------------");

    int row = 4;
    int total_blocks = mcb_.MemMgr.get_total_blocks();
    int page_size = mcb_.MemMgr.get_page_size();

    for (int i = 0; i < total_blocks && row <= 9; )
    {
        int handle = mcb_.MemMgr.get_block_handle(i);
        int start_block = i;

        while (i + 1 < total_blocks && mcb_.MemMgr.get_block_handle(i + 1) == handle)
            i++;

        int end_block = i;

        int start_loc = start_block * page_size;
        int end_loc = ((end_block + 1) * page_size) - 1;
        int size_bytes = (end_block - start_block + 1) * page_size;

        if (handle == 0)
        {
            mvwprintw(memory_usage_win_, row, 2,
                      "%-8s %-13s %-10d %-10d %-11d %-16s %-8s",
                      "Free", "-", start_loc, end_loc, size_bytes, "NA", "MMU");
        }
        else
        {
            int task_id = find_task_for_handle(handle);
            char current_loc[16];
            char task_text[16];

            if (task_id >= 0)
            {
                tcb* task = mcb_.Swapper.get_task(task_id);

                int cursor = 0;
                if (task)
                {
                    for (int j = 0; j < (int)task->memory_regions.size(); j++)
                    {
                        if (task->memory_regions[j].handle == handle)
                        {
                            cursor = task->memory_regions[j].cursor;
                            break;
                        }
                    }
                }

                snprintf(current_loc, sizeof(current_loc), "%d", start_loc + cursor);
                snprintf(task_text, sizeof(task_text), "%d", task_id);
            }
            else
            {
                snprintf(current_loc, sizeof(current_loc), "NA");
                snprintf(task_text, sizeof(task_text), "?");
            }

            mvwprintw(memory_usage_win_, row, 2,
                      "%-8s %-13d %-10d %-10d %-11d %-16s %-8s",
                      "Used", handle, start_loc, end_loc, size_bytes, current_loc, task_text);
        }

        row++;
        i++;
    }

    if (row == 4)
        mvwprintw(memory_usage_win_, row, 2, "(no memory blocks to display)");

    wrefresh(memory_usage_win_);
}

// draw_core_dump()
void ULTIMA::draw_core_dump()
{
    werase(core_dump_win_);
    box(core_dump_win_, 0, 0);
    mvwprintw(core_dump_win_, 1, 2, "Memory CORE Dump:");

    const unsigned char* mem = mcb_.MemMgr.get_memory();
    int mem_size = mcb_.MemMgr.get_memory_size();
    int bytes_per_line = 16;

    int max_y, max_x;
    getmaxyx(core_dump_win_, max_y, max_x);
    (void)max_x;

    int row = 2;

    for (int offset = 0; offset < mem_size && row < max_y - 1; offset += bytes_per_line)
    {
        char hex_part[80] = {0};
        char ascii_part[20] = {0};
        int hex_index = 0;
        int ascii_index = 0;

        for (int i = 0; i < bytes_per_line && (offset + i) < mem_size; i++)
        {
            unsigned char value = mem[offset + i];

            hex_index += snprintf(hex_part + hex_index,
                                  sizeof(hex_part) - hex_index,
                                  "%02X ", value);

            ascii_part[ascii_index++] = (value >= 32 && value <= 126) ? (char)value : '.';
        }

        ascii_part[ascii_index] = '\0';

        mvwprintw(core_dump_win_, row, 2, "%04d: %-48s %s", offset, hex_part, ascii_part);
        row++;
    }

    wrefresh(core_dump_win_);
}

// draw_log()
void ULTIMA::draw_log(const std::string& line)
{
    if (log_win_ == nullptr)
        return;

    int max_y, max_x;
    getmaxyx(log_win_, max_y, max_x);
    (void)max_x;

    if (log_line_ >= max_y - 1)
    {
        werase(log_win_);
        box(log_win_, 0, 0);
        mvwprintw(log_win_, 1, 2, "Log:");
        log_line_ = 2;
    }

    if (log_line_ == 1)
    {
        werase(log_win_);
        box(log_win_, 0, 0);
        mvwprintw(log_win_, 1, 2, "Log:");
        log_line_ = 2;
    }

    mvwprintw(log_win_, log_line_, 2, "%s", line.c_str());
    log_line_++;
    wrefresh(log_win_);
}

// draw_console()
void ULTIMA::draw_console()
{
    werase(console_win_);
    box(console_win_, 0, 0);

    mvwprintw(console_win_, 1, 2, "Console Commands");
    mvwprintw(console_win_, 2, 2, "p/r : pause/resume");
    mvwprintw(console_win_, 3, 2, "a/f/z: allocate/free/free-only");
    mvwprintw(console_win_, 4, 2, "w/x : write/read");
    mvwprintw(console_win_, 5, 2, "k/g/q : kill/gc/quit");
    mvwprintw(console_win_, 6, 2, "h : switch memory region");
    mvwprintw(console_win_, 7, 2, "f = free+coalesce");
    mvwprintw(console_win_, 8, 2, "z = free only");

    wrefresh(console_win_);
}

// draw_all()
void ULTIMA::draw_all()
{
    draw_heading();
    draw_tasks();
    draw_semaphore();
    draw_mailboxes();
    draw_memory_usage();
    draw_core_dump();
    draw_console();

    if (log_win_ != nullptr)
    {
        box(log_win_, 0, 0);
        mvwprintw(log_win_, 1, 2, "Log:");
        wrefresh(log_win_);
    }
}

// waste_time()
void ULTIMA::waste_time(int factor)
{
    volatile long counter = 0;
    for (long i = 0; i < factor * 1000000L; i++)
        counter += i % 3;
}

// handle_input()
void ULTIMA::handle_input(int ch)
{
    switch (ch)
    {
        case 'm':
        {
            int start = selected_mailbox_;
            bool found = false;

            do
            {
                selected_mailbox_++;
                if (selected_mailbox_ >= mcb_.Swapper.get_task_count())
                    selected_mailbox_ = 0;

                tcb* node = mcb_.Swapper.get_current();
                if (node)
                {
                    tcb* first = node;
                    do
                    {
                        if (node->task_id == selected_mailbox_)
                        {
                            found = true;
                            break;
                        }
                        node = node->next;
                    } while (node != first);
                }

                if (found) break;

            } while (selected_mailbox_ != start);

            draw_log("Switched mailbox view.");
            break;
        }

        case 't':
            selected_semaphore_ = (selected_semaphore_ + 1) % 2;
            draw_log("Toggled semaphore view.");
            break;

        case 's':
        {
            int source_task = mcb_.Swapper.get_task_id();
            int dest_task = selected_mailbox_;

            if (source_task == dest_task)
            {
                draw_log("Task cannot send a message to itself.");
                break;
            }

            ipc::Message* msg = new ipc::Message;
            msg->Source_Task_Id = source_task;
            msg->Destination_Task_Id = dest_task;
            msg->Msg_Type.Message_Type_Id = 0;
            msg->Msg_Type.Message_Type_Description = "Text";
            char buffer[MAX_MSG_SIZE];
            snprintf(buffer, MAX_MSG_SIZE,
                     "Message from task %d to task %d.",
                     source_task, dest_task);
            msg->plaintext = buffer;

            if (mcb_.Messenger.Message_Send(msg) == 1) {
                std::string cipher_preview = hex_preview(msg->packet.cipher);

                draw_log("Encrypted payload: " + cipher_preview);
                draw_log("Text message sent.");
            }
            else
            {
                delete msg;
                draw_log("Mailbox full. Message failed to send.");
            }
            break;
        }

        case 'v':
        {
            int source_task = mcb_.Swapper.get_task_id();
            int dest_task = selected_mailbox_;

            if (source_task == dest_task)
            {
                draw_log("Task cannot send a message to itself.");
                break;
            }

            ipc::Message* msg = new ipc::Message;
            msg->Source_Task_Id = source_task;
            msg->Destination_Task_Id = dest_task;
            msg->Msg_Type.Message_Type_Id = 1;
            msg->Msg_Type.Message_Type_Description = "Service";
            msg->plaintext = "lpr file1";

            if (mcb_.Messenger.Message_Send(msg) == 1) {
                std::string cipher_preview = hex_preview(msg->packet.cipher);

                draw_log("Encrypted payload: " + cipher_preview);
                draw_log("Service message sent.");
            }
            else
            {
                delete msg;
                draw_log("Mailbox full. Message failed to send.");
            }
            break;
        }

        case 'n':
        {
            int source_task = mcb_.Swapper.get_task_id();
            int dest_task = selected_mailbox_;

            if (source_task == dest_task)
            {
                draw_log("Task cannot send a message to itself.");
                break;
            }

            ipc::Message* msg = new ipc::Message;
            msg->Source_Task_Id = source_task;
            msg->Destination_Task_Id = dest_task;
            msg->Msg_Type.Message_Type_Id = 2;
            msg->Msg_Type.Message_Type_Description = "Notification";
            msg->plaintext = "Notify: service request queued.";

            if (mcb_.Messenger.Message_Send(msg) == 1) {
                std::string cipher_preview = hex_preview(msg->packet.cipher);

                draw_log("Encrypted payload: " + cipher_preview);
                draw_log("Notification sent.");
            }
            else
            {
                delete msg;
                draw_log("Mailbox full. Message failed to send.");
            }
            break;
        }

        case 'p':
            paused_ = true;
            draw_log("System paused.");
            break;

        case 'r':
            paused_ = false;
            draw_log("System resumed.");
            break;

        case 'd':
        {
            Semaphore* current_sema = (selected_semaphore_ == 0) ? &mcb_.Monitor : &mcb_.Printer;
            draw_log("Semaphore down requested.");
            current_sema->down(mcb_.Swapper.get_task_id());
            break;
        }

        case 'u':
        {
            Semaphore* current_sema = (selected_semaphore_ == 0) ? &mcb_.Monitor : &mcb_.Printer;
            draw_log("Semaphore up requested.");
            current_sema->up();
            break;
        }

        case 'a':
        {
            tcb* task = mcb_.Swapper.get_current();
            if (task == nullptr)
                break;

            int request_size = 64;
            int handle = mcb_.MemMgr.Mem_Alloc(request_size);

            if (handle < 0)
            {
                draw_log("Memory allocation failed.");
                break;
            }

            task_memory_region region;
            region.handle = handle;
            region.size = request_size;
            region.cursor = 0;

            task->memory_regions.push_back(region);
            task->active_region_index = (int)task->memory_regions.size() - 1;

            char log_buffer[64];
            snprintf(log_buffer, sizeof(log_buffer),
                     "Task %d allocated %d bytes. Handle=%d",
                     task->task_id, request_size, handle);
            draw_log(log_buffer);
            break;
        }

        case 'f':
        {
            tcb* task = mcb_.Swapper.get_current();

            if (task == nullptr || task->memory_regions.empty() ||
                task->active_region_index < 0 ||
                task->active_region_index >= (int)task->memory_regions.size())
            {
                draw_log("Current task has no active memory region.");
                break;
            }

            int idx = task->active_region_index;
            int handle = task->memory_regions[idx].handle;

            if (mcb_.MemMgr.Mem_Free(handle) == 0)
            {
                task->memory_regions.erase(task->memory_regions.begin() + idx);

                if (task->memory_regions.empty())
                    task->active_region_index = -1;
                else if (idx >= (int)task->memory_regions.size())
                    task->active_region_index = (int)task->memory_regions.size() - 1;

                draw_log("Memory freed and coalesced for current task.");
            }
            else
            {
                draw_log("Memory free failed.");
            }
            break;
        }

        case 'z':
        {
            tcb* task = mcb_.Swapper.get_current();

            if (task == nullptr || task->memory_regions.empty() ||
                task->active_region_index < 0 ||
                task->active_region_index >= (int)task->memory_regions.size())
            {
                draw_log("Current task has no active memory region.");
                break;
            }

            int idx = task->active_region_index;
            int handle = task->memory_regions[idx].handle;

            if (mcb_.MemMgr.Mem_Free_NoCoalesce(handle) == 0)
            {
                task->memory_regions.erase(task->memory_regions.begin() + idx);

                if (task->memory_regions.empty())
                    task->active_region_index = -1;
                else if (idx >= (int)task->memory_regions.size())
                    task->active_region_index = (int)task->memory_regions.size() - 1;

                draw_log("Memory freed without coalesce.");
            }
            else
            {
                draw_log("Memory free without coalesce failed.");
            }
            break;
        }

        case 'w':
        {
            tcb* task = mcb_.Swapper.get_current();

            if (task == nullptr || task->memory_regions.empty() ||
                task->active_region_index < 0 ||
                task->active_region_index >= (int)task->memory_regions.size())
            {
                draw_log("Allocate memory before writing.");
                break;
            }

            task_memory_region& region = task->memory_regions[task->active_region_index];
            int handle = region.handle;

            char text[64];
            snprintf(text, sizeof(text), "this is task %d", task->task_id);
            int text_size = (int)strlen(text);

            if (mcb_.MemMgr.Mem_Write(handle, 0, text_size, text) == 0)
            {
                region.cursor = text_size;
                draw_log("Write to memory.");
            }
            else
            {
                draw_log("Memory write failed.");
            }
            break;
        }

        case 'x':
        {
            tcb* task = mcb_.Swapper.get_current();

            if (task == nullptr || task->memory_regions.empty() ||
                task->active_region_index < 0 ||
                task->active_region_index >= (int)task->memory_regions.size())
            {
                draw_log("Allocate memory before reading.");
                break;
            }

            task_memory_region& region = task->memory_regions[task->active_region_index];
            int handle = region.handle;

            char text[64];
            memset(text, 0, sizeof(text));

            int read_size = region.cursor;
            if (read_size <= 0) read_size = 32;
            if (read_size > (int)sizeof(text) - 1) read_size = sizeof(text) - 1;

            if (mcb_.MemMgr.Mem_Read(handle, 0, read_size, text) == 0)
            {
                text[read_size] = '\0';
                std::string line = "Read from memory: ";
                line += text;
                draw_log(line);
            }
            else
            {
                draw_log("Memory read failed.");
            }
            break;
        }

        case 'h':
        {
            tcb* task = mcb_.Swapper.get_current();

            if (task == nullptr || task->memory_regions.empty())
            {
                draw_log("Current task has no memory regions.");
                break;
            }

            task->active_region_index++;
            if (task->active_region_index >= (int)task->memory_regions.size())
                task->active_region_index = 0;

            char log_buffer[128];
            snprintf(log_buffer, sizeof(log_buffer),
                     "Task %d active handle=%d",
                     task->task_id,
                     task->memory_regions[task->active_region_index].handle);
            draw_log(log_buffer);
            break;
        }

        case 'k':
        {
            int current_task = mcb_.Swapper.get_task_id();
            Semaphore* current_sema = (selected_semaphore_ == 0) ? &mcb_.Monitor : &mcb_.Printer;

            draw_log("Kill task requested.");

            if (current_sema->get_lucky_task() == current_task)
            {
                draw_log("Current task owns resource. Releasing resource first.");
                current_sema->up();
            }

            tcb* task = mcb_.Swapper.get_current();
            if (task != nullptr)
            {
                free_task_memory(task);
                draw_log("Killed task memory released.");
            }

            mcb_.Swapper.kill();
            draw_log("Task killed.");
            break;
        }

        case 'g':
        {
            draw_log("Garbage collection requested.");

            for (int i = 0; i < mcb_.Swapper.get_task_count(); i++)
            {
                if (mcb_.Swapper.get_state(i) == DEAD)
                    mcb_.Messenger.Message_DeleteAll(i);
            }

            mcb_.Swapper.garbage();

            selected_mailbox_ = mcb_.Swapper.get_task_id();
            if (selected_mailbox_ < 0)
                selected_mailbox_ = 0;

            draw_log("Garbage collection complete.");
            break;
        }

        case 'q':
            running_ = false;
            break;

        case ERR:
            break;

        default:
            draw_log("Unknown command.");
            break;
    }
}

// run()
void ULTIMA::run()
{
    init_curses();
    initialize_scheduler();
    draw_all();
    draw_log("Ready for input.");

    while (running_)
    {
        int ch = wgetch(stdscr);

        if (ch != ERR)
            handle_input(ch);

        if (!paused_)
        {
            waste_time(10);
            mcb_.Swapper.yield();

            int running_task = mcb_.Swapper.get_task_id();

            if (running_task >= 0)
            {
                ipc::Message received_msg;
                int result = mcb_.Messenger.Message_Receive(running_task, &received_msg);

                if (result == 1)
                {
                    std::string cipher_preview = hex_preview(received_msg.packet.cipher);

                    char log_buffer[256];
                    snprintf(log_buffer, sizeof(log_buffer),
                             "RX Task %d | ENC: %s | DEC: %s",
                             running_task,
                             cipher_preview.c_str(),
                             received_msg.plaintext.c_str());

                    draw_log(log_buffer);
                }
            }
        }

        draw_all();
        usleep(150000);
    }
}

// main()
int main()
{
    ULTIMA app;
    app.run();
    return 0;
}
