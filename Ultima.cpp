/* Code Framework built from Lab 6
Primary Author: Dylan Hurt*/
#include "Ultima.h"

#include <cstdio>
#include <string>
#include <unistd.h>

// ULTIMA Constructor
// Initializes all window pointers and UI state
ULTIMA::ULTIMA()
    : heading_win_(nullptr),
    task_win_(nullptr),
    sema_win_(nullptr),
    log_win_(nullptr),
    console_win_(nullptr),
    mailbox_win_(nullptr),
    running_(true),
    paused_(false),
    log_line_(1)
{
    // Create tasks first
    draw_log("Scheduler started.");
    for (int i = 0; i < MAX_TASKS; i++) {

        mcb_.Swapper.create_task();
    }

    mcb_.Swapper.start();

    // Initialize IPC for the tasks
    mcb_.InitIPC();
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

    heading_win_ = create_window(6, 96, 1, 2);
    task_win_    = create_window(12, 52, 8, 2);
    sema_win_    = create_window(12, 52, 8, 56);
    log_win_     = create_window(12, 72, 21, 2);
    console_win_ = create_window(12, 34, 21, 75);
    mailbox_win_ = create_window(6, 25, 1, 100);

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
    mvwprintw(heading_win_, 3, 2, "Quantum: %ld", quantum);
    mvwprintw(heading_win_, 4, 2, "Elapsed: %ld   Remaining: %ld", (long)elapsed, remaining);

    wrefresh(heading_win_);
}

// draw_tasks()
// Draws the process table window
void ULTIMA::draw_tasks()
{
    werase(task_win_);
    box(task_win_, 0, 0);

    mvwprintw(task_win_, 1, 2, "Process Table Dump:");
    mvwprintw(task_win_, 3, 2,
              "%-12s %-8s %-10s %-8s",
              "TaskName", "TaskID", "State", "Etc.");
    mvwprintw(task_win_, 4, 2,
              "----------------------------------------------");

    int line = 5;

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

    mvwprintw(sema_win_, 1, 2, "Semaphore Dump:");
    mvwprintw(sema_win_, 3, 2, "Resource: %s", mcb_.Monitor.get_resource_name().c_str());
    mvwprintw(sema_win_, 4, 2, "Sema_value: %d", mcb_.Monitor.get_sema_value());
    mvwprintw(sema_win_, 5, 2, "Sema_queue: %s", mcb_.Monitor.get_queue_string().c_str());
    mvwprintw(sema_win_, 6, 2, "Lucky_task: %d", mcb_.Monitor.get_lucky_task());

    wrefresh(sema_win_);
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
        log_line_ = 1;
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
    mvwprintw(console_win_, 3, 2, "p : Pause");
    mvwprintw(console_win_, 4, 2, "r : Resume");
    mvwprintw(console_win_, 5, 2, "d : Request resource");
    mvwprintw(console_win_, 6, 2, "u : Release resource");
    mvwprintw(console_win_, 7, 2, "k : Kill current task");
    mvwprintw(console_win_, 8, 2, "g : Garbage collect");
    mvwprintw(console_win_, 9, 2, "q : Quit");

    wrefresh(console_win_);
}
// Draws mailbox window
void ULTIMA::draw_mailbox()
{
    werase(mailbox_win_);
    box(mailbox_win_, 0, 0);

    mvwprintw(mailbox_win_, 1, 2, "Mailbox Dump:");

    for (int i = 0; i < MAX_TASKS; i++)
    {
        mvwprintw(mailbox_win_, 2+i, 2, "Task %d: %s", i,
                  mcb_.Messenger.Message_Count(i) > 0 ? "Has messages" : "Empty");
    }

    wrefresh(mailbox_win_);
}

// draw_all()
// Redraws all windows on the screen
void ULTIMA::draw_all()
{
    draw_heading();
    draw_tasks();
    draw_semaphore();
    draw_console();
    draw_mailbox();
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
        case 'p':
            paused_ = true;
            draw_log("System paused.");
            break;

        case 'r':
            paused_ = false;
            draw_log("System resumed.");
            break;

        case 'd':
            draw_log("Semaphore down requested.");
            mcb_.Monitor.down(mcb_.Swapper.get_task_id());
            break;

        case 'u':
            draw_log("Semaphore up requested.");
            mcb_.Monitor.up();
            break;

        case 'k':
        {
            int current_task = mcb_.Swapper.get_task_id();

            draw_log("Kill task requested.");

            if (mcb_.Monitor.get_lucky_task() == current_task)
            {
                draw_log("Current task owns resource. Releasing resource first.");
                mcb_.Monitor.up();
            }

            mcb_.Swapper.kill();
            draw_log("Task killed.");
            break;
        }

        case 'g':
            draw_log("Garbage collection requested.");
            mcb_.Swapper.garbage();
            draw_log("Garbage collection complete.");
            break;

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
    draw_all();
    draw_log("Ready for input.");

    while (running_)
    {
        int ch = wgetch(stdscr);

        if (ch != ERR)
        {
            handle_input(ch);
        }

        // Free-flowing execution while not paused
        if (!paused_)
        {
            waste_time(10);
            mcb_.Swapper.yield();
        }

        draw_all();

        // Slow the loop so the UI is readable
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