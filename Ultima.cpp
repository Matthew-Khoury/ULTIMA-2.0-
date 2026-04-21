/* Code Framework built from Lab 6
Primary Author: Dylan Hurt
Memory window integration update.
*/
#include "Ultima.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <unistd.h>

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
    for (int i = 0; i < MAX_TASKS; i++)
    {
        task_memory_handle_[i] = -1;
        task_memory_request_size_[i] = 0;
        task_memory_cursor_[i] = 0;
    }
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

    heading_win_      = create_window(5, 132, 1, 2);
    task_win_         = create_window(9, 64, 7, 2);
    sema_win_         = create_window(9, 65, 7, 67);
    mailbox_win_      = create_window(15, 130, 17, 2);
    memory_usage_win_ = create_window(11, 130, 33, 2);
    core_dump_win_    = create_window(18, 130, 45, 2);
    log_win_          = create_window(12, 88, 64, 2);
    console_win_      = create_window(12, 40, 64, 91);

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
    draw_log("Scheduler started.");

    for (int i = 0; i < MAX_TASKS; i++)
        mcb_.Swapper.create_task();

    mcb_.Swapper.start();
    mcb_.InitIPC();

    draw_log("IPC initialized.");
    draw_log("Memory manager initialized (1024 bytes, 128-byte pages).");
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
        char handle_text[16];
        if (task_memory_handle_[i] >= 0)
            snprintf(handle_text, sizeof(handle_text), "%d", task_memory_handle_[i]);
        else
            snprintf(handle_text, sizeof(handle_text), "None");

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
        mvwprintw(mailbox_win_, 13, 2, "m : switch / s : text / v : service / n : notification");
        wrefresh(mailbox_win_);
        return;
    }

    int message_count = mcb_.Messenger.Message_Count(task_id);
    if (message_count < 0) message_count = 0;

    mvwprintw(mailbox_win_, 3, 2, "Message Count: %d", message_count);
    mvwprintw(mailbox_win_, 5, 2, "Src  Dst  Message                            Size   Type            Time");
    mvwprintw(mailbox_win_, 6, 2, "--------------------------------------------------------------------------------");

    if (message_count == 0)
    {
        mvwprintw(mailbox_win_, 7, 2, "(empty)");
    }
    else
    {
        tcb* task = mcb_.Swapper.get_task(task_id);
        if (task != nullptr)
        {
            int row = 7;

            for (int i = 0; i < task->mailbox.Size() && row <= 12; i++)
            {
                ipc::Message* msg = (ipc::Message*)(intptr_t)task->mailbox.Peek(i);

                if (msg != nullptr)
                {
                    char time_buffer[16];
                    struct tm* tm_info = localtime(&msg->Message_Arrival_Time);
                    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", tm_info);

                    mvwprintw(mailbox_win_, row, 2,
                              "%-4d %-4d %-34.34s %-6d %-15.15s %-10s",
                              msg->Source_Task_Id,
                              msg->Destination_Task_Id,
                              msg->Msg_Text,
                              msg->Msg_Size,
                              msg->Msg_Type.Message_Type_Description,
                              time_buffer);
                    row++;
                }
            }
        }
    }

    mvwprintw(mailbox_win_, 13, 2, "m : switch / s : text / v : service / n : notification");
    wrefresh(mailbox_win_);
}

// helper
// TODO: this is a potential bug that needs fixed for displaying the current_location and taskID of the allocated block of memory
int ULTIMA::find_task_for_handle(int handle) const
{
    if (handle <= 0) return -1;

    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (task_memory_handle_[i] == handle)
            return i;
    }
    return -1;
}

// helper
int ULTIMA::active_memory_handle_count() const
{
    int count = 0;
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (task_memory_handle_[i] >= 0)
            count++;
    }
    return count;
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
                snprintf(current_loc, sizeof(current_loc), "%d", start_loc + task_memory_cursor_[task_id]);
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
            snprintf(msg->Msg_Text, MAX_MSG_SIZE,
                     "Message from task %d to task %d.",
                     source_task, dest_task);

            if (mcb_.Messenger.Message_Send(msg) == 1)
                draw_log("Text message sent.");
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
            strncpy(msg->Msg_Text, "lpr file1", MAX_MSG_SIZE - 1);
            msg->Msg_Text[MAX_MSG_SIZE - 1] = '\0';

            if (mcb_.Messenger.Message_Send(msg) == 1)
                draw_log("Service message sent.");
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
            strncpy(msg->Msg_Text, "Got your print service request", MAX_MSG_SIZE - 1);
            msg->Msg_Text[MAX_MSG_SIZE - 1] = '\0';

            if (mcb_.Messenger.Message_Send(msg) == 1)
                draw_log("Notification sent.");
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
            int current_task = mcb_.Swapper.get_task_id();

            if (current_task < 0)
                break;

            // /*
            // if (task_memory_handle_[current_task] >= 0)
            // {
            //     draw_log("Current task already owns a memory handle.");
            //     break;
            // } commented out for testing
            // */

            int request_size = 64; // TODO: test changing this value
            int handle = mcb_.MemMgr.Mem_Alloc(request_size);

            if (handle < 0)
            {
                draw_log("Memory allocation failed.");
                break;
            }

            task_memory_handle_[current_task] = handle;
            task_memory_request_size_[current_task] = request_size;
            task_memory_cursor_[current_task] = 0;

            char log_buffer[128];
            snprintf(log_buffer, sizeof(log_buffer),
                     "Task %d allocated %d bytes. Handle=%d",
                     current_task, request_size, handle);
            draw_log(log_buffer);
            break;
        }

        case 'f':
        {
            int current_task = mcb_.Swapper.get_task_id();

            if (current_task < 0)
                break;

            int handle = task_memory_handle_[current_task];
            if (handle < 0)
            {
                draw_log("Current task has no memory to free.");
                break;
            }

            if (mcb_.MemMgr.Mem_Free(handle) == 0)
            {
                task_memory_handle_[current_task] = -1;
                task_memory_request_size_[current_task] = 0;
                task_memory_cursor_[current_task] = 0;
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
            int current_task = mcb_.Swapper.get_task_id();

            if (current_task < 0)
                break;

            int handle = task_memory_handle_[current_task];
            if (handle < 0)
            {
                draw_log("Current task has no memory to free.");
                break;
            }

            if (mcb_.MemMgr.Mem_Free_NoCoalesce(handle) == 0)
            {
                task_memory_handle_[current_task] = -1;
                task_memory_request_size_[current_task] = 0;
                task_memory_cursor_[current_task] = 0;
                draw_log("Memory freed without coalesce. #### visible in dump.");
            }
            else
            {
                draw_log("Memory free without coalesce failed.");
            }
            break;
        }

        case 'w':
        {
            int current_task = mcb_.Swapper.get_task_id();

            if (current_task < 0)
                break;

            int handle = task_memory_handle_[current_task];
            if (handle < 0)
            {
                draw_log("Allocate memory before writing.");
                break;
            }

            char text[64];
            snprintf(text, sizeof(text), "this is task %d", current_task);
            int text_size = (int)strlen(text);

            if (mcb_.MemMgr.Mem_Write(handle, 0, text_size, text) == 0)
            {
                task_memory_cursor_[current_task] = text_size;
                draw_log("Sample text written to task memory.");
            }
            else
            {
                draw_log("Memory write failed.");
            }
            break;
        }

        case 'x':
        {
            int current_task = mcb_.Swapper.get_task_id();

            if (current_task < 0)
                break;

            int handle = task_memory_handle_[current_task];
            if (handle < 0)
            {
                draw_log("Allocate memory before reading.");
                break;
            }

            char text[64];
            memset(text, 0, sizeof(text));

            int read_size = task_memory_cursor_[current_task];
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

            if (current_task >= 0 && task_memory_handle_[current_task] >= 0)
            {
                mcb_.MemMgr.Mem_Free(task_memory_handle_[current_task]);
                task_memory_handle_[current_task] = -1;
                task_memory_request_size_[current_task] = 0;
                task_memory_cursor_[current_task] = 0;
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
                    char log_buffer[160];
                    snprintf(log_buffer, sizeof(log_buffer),
                             "Task %d received: %s",
                             running_task,
                             received_msg.Msg_Text);
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