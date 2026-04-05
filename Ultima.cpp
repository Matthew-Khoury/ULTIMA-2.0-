/* Code Framework built from Lab 6
Primary Author: Dylan Hurt
*/
#include "Ultima.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <string>
#include <unistd.h>

// ULTIMA Constructor
// Initializes all window pointers and UI state
ULTIMA::ULTIMA()
    : heading_win_(nullptr),
      task_win_(nullptr),
      sema_win_(nullptr),
      mailbox_win_(nullptr),
      log_win_(nullptr),
      console_win_(nullptr),
      mcb_(),
      running_(true),
      paused_(false),
      log_line_(1),
      selected_mailbox_(0),
      selected_semaphore_(0)
{
}

// Destructor
// Makes sure ncurses shuts down cleanly
ULTIMA::~ULTIMA()
{
    shutdown_curses();
}

// init_curses()
// Starts ncurses mode and creates all interface windows
void ULTIMA::init_curses()
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);

    // non-blocking input so the simulation can keep running
    nodelay(stdscr, TRUE);

    refresh();

    heading_win_ = create_window(5, 106, 1, 2);
    task_win_    = create_window(9, 52, 7, 2);
    sema_win_    = create_window(9, 52, 7, 56);

    // taller mailbox window
    mailbox_win_ = create_window(15, 106, 17, 2);

    // move log and console down and make them taller
    log_win_     = create_window(14, 72, 32, 2);
    console_win_ = create_window(14, 34, 32, 75);

    draw_log("----- ULTIMA Log Started -----");
}

// shutdown_curses()
// Ends ncurses mode
void ULTIMA::shutdown_curses()
{
    if (stdscr != nullptr)
    {
        endwin();
    }
}

// create_window()
// Creates an ncurses window and draws it
WINDOW* ULTIMA::create_window(int height, int width, int y, int x)
{
    WINDOW* win = newwin(height, width, y, x);
    box(win, 0, 0);
    wrefresh(win);
    return win;
}

// initialize_scheduler()
// Start the scheduler and create tasks
void ULTIMA::initialize_scheduler()
{
    draw_log("Scheduler started.");

    for (int i = 0; i < MAX_TASKS; i++)
    {
        mcb_.Swapper.create_task();
    }

    mcb_.Swapper.start();
    mcb_.InitIPC();

    draw_log("IPC initialized.");
}

// draw_heading()
// Draws the heading window
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

        if (remaining < 0)
        {
            remaining = 0;
        }
    }

    mvwprintw(heading_win_, 1, 2, "ULTIMA Project");
    mvwprintw(heading_win_, 2, 2, "Current Task ID: %d", current_task_id);
    mvwprintw(heading_win_, 3, 2, "Quantum: %ld   Elapsed: %ld   Remaining: %ld",
              quantum, (long)elapsed, remaining);

    wrefresh(heading_win_);
}

// draw_tasks()
// Draws the process table window
void ULTIMA::draw_tasks()
{
    werase(task_win_);
    box(task_win_, 0, 0);

    mvwprintw(task_win_, 1, 2, "Process Table Dump:");
    mvwprintw(task_win_, 2, 2,
              "%-12s %-8s %-10s %-8s",
              "TaskName", "TaskID", "State", "Etc.");
    mvwprintw(task_win_, 3, 2,
              "----------------------------------------------");

    int line = 4;

    // Loop over all tasks
    for (int i = 0; i < mcb_.Swapper.get_task_count(); i++)
    {
        std::string task_name = "Task " + std::to_string(i);
        std::string etc_text;

        etc_text = std::to_string((int)mcb_.Swapper.get_elapsed_time(i));

        // Show "Running" for the current task
        if (i == mcb_.Swapper.get_task_id() && mcb_.Swapper.get_state(i) != DEAD)
        {
            etc_text = "Running";
        }

        // Check if task is physically in the circular list
        // Only skip it if it has been removed by garbage()
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

        mvwprintw(task_win_, line, 2,
                  "%-12s %-8d %-10s %-8s",
                  task_name.c_str(),
                  i,
                  mcb_.Swapper.get_state(i).c_str(),
                  etc_text.c_str());
        line++;
    }
    wrefresh(task_win_);
}

// draw_semaphore()
// Draws the semaphore window
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
    mvwprintw(sema_win_, 7, 2, "Press 't' to toggle resource.");

    wrefresh(sema_win_);
}

// draw_mailboxes()
// Draws the mailbox dump
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
        mvwprintw(mailbox_win_, 13, 2, "m:switch  s/v/n:send");
        wrefresh(mailbox_win_);
        return;
    }

    int message_count = mcb_.Messenger.Message_Count(task_id);

    if (message_count < 0)
    {
        message_count = 0;
    }

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

    mvwprintw(mailbox_win_, 13, 2, "m:switch  s/v/n:send");

    wrefresh(mailbox_win_);
}

// draw_log()
// Displays messages in the log window
void ULTIMA::draw_log(const std::string& line)
{
    if (log_win_ == nullptr)
        return;

    int max_y, max_x;
    getmaxyx(log_win_, max_y, max_x);

    if (log_line_ >= max_y - 1)
    {
        werase(log_win_);
        box(log_win_, 0, 0);
        mvwprintw(log_win_, 1, 2, "Log:");
        log_line_ = 2;
    }

    // Make sure header is always present
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
// Draws the command help window for keyboard input
void ULTIMA::draw_console()
{
    werase(console_win_);
    box(console_win_, 0, 0);

    mvwprintw(console_win_, 1, 2, "Console Commands");
    mvwprintw(console_win_, 2, 2, "m : Next mailbox");
    mvwprintw(console_win_, 3, 2, "t : Toggle semaphore");
    mvwprintw(console_win_, 4, 2, "s : Send text message ");
    mvwprintw(console_win_, 5, 2, "v : Send service message");
    mvwprintw(console_win_, 6, 2, "n : Send notify message");
    mvwprintw(console_win_, 7, 2, "p/r : Pause / Resume");
    mvwprintw(console_win_, 8, 2, "d/u : Down / Up");
    mvwprintw(console_win_, 9, 2, "k/g/q : Kill / GC / Quit");

    wrefresh(console_win_);
}

// draw_all()
// Redraws all windows on the screen
void ULTIMA::draw_all()
{
    draw_heading();
    draw_tasks();
    draw_semaphore();
    draw_mailboxes();
    draw_console();

    // Keep log box visible even when no new message arrives
    if (log_win_ != nullptr)
    {
        box(log_win_, 0, 0);
        mvwprintw(log_win_, 1, 2, "Log:");
        wrefresh(log_win_);
    }
}

void ULTIMA::waste_time(int factor)
{
    volatile long counter = 0;
    for (long i = 0; i < factor * 1000000L; i++)
    {
        counter += i % 3;
    }
}

// handle_input()
// Handles keyboard inputs from the console
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
            selected_semaphore_++;
            if (selected_semaphore_ > 1)
                selected_semaphore_ = 0;
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
                {
                    mcb_.Messenger.Message_DeleteAll(i);
                }
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
// Starts ncurses, draws the UI, and keeps handling input
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
        {
            handle_input(ch);
        }

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