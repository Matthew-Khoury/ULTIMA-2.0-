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
      scheduler_(),
      resource1_(1, "Resource1", &scheduler_),
      running_(true),
      log_line_(1)
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
    nodelay(stdscr, FALSE);

    refresh();

    heading_win_ = create_window(6, 96, 1, 2);
    task_win_    = create_window(12, 52, 8, 2);
    sema_win_    = create_window(12, 52, 8, 56);
    log_win_     = create_window(12, 72, 21, 2);
    console_win_ = create_window(12, 34, 21, 75);

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
        scheduler_.create_task();
    }

    scheduler_.start();
}

// draw_heading()
// Draws the heading window
void ULTIMA::draw_heading()
{
    werase(heading_win_);
    box(heading_win_, 0, 0);

    int current_task_id = scheduler_.get_task_id();
    long quantum = scheduler_.get_quantum();
    clock_t elapsed = 0;
    long remaining = quantum;

    if (current_task_id >= 0)
    {
        elapsed = scheduler_.get_elapsed_time(current_task_id);
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

    for (int i = 0; i < scheduler_.get_task_count(); i++)
    {
        std::string task_name = "Task " + std::to_string(i);
        std::string etc_text;

        if (i == scheduler_.get_task_id() && scheduler_.get_state(i) != DEAD)
        {
            etc_text = "Running";
        }
        else
        {
            etc_text = std::to_string((int)scheduler_.get_elapsed_time(i));
        }

        mvwprintw(task_win_, 5 + i, 2,
                  "%-12s %-8d %-10s %-8s",
                  task_name.c_str(),
                  i,
                  scheduler_.get_state(i).c_str(),
                  etc_text.c_str());
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
    mvwprintw(sema_win_, 3, 2, "Resource: %s", resource1_.get_resource_name().c_str());
    mvwprintw(sema_win_, 4, 2, "Sema_value: %d", resource1_.get_sema_value());
    mvwprintw(sema_win_, 5, 2, "Sema_queue: %s", resource1_.get_queue_string().c_str());
    mvwprintw(sema_win_, 6, 2, "Lucky_task: %d", resource1_.get_lucky_task());

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
    mvwprintw(console_win_, 3, 2, "n : Yield to next task");
    mvwprintw(console_win_, 4, 2, "d : Request resource");
    mvwprintw(console_win_, 5, 2, "u : Release resource");
    mvwprintw(console_win_, 6, 2, "w : Waste CPU time");
    mvwprintw(console_win_, 7, 2, "k : Kill current task");
    mvwprintw(console_win_, 8, 2, "g : Garbage collect");
    mvwprintw(console_win_, 9, 2, "q : Quit");

    wrefresh(console_win_);
}

// draw_all()
// Redraws all windows on the screen
void ULTIMA::draw_all()
{
    draw_heading();
    draw_tasks();
    draw_semaphore();
    draw_console();
}

void ULTIMA::waste_time(int factor)
{
    volatile long counter = 0;
    for (long i = 0; i < factor * 10000000L; i++)
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
        case 'n':
        {
            int before = scheduler_.get_task_id();

            draw_log("Task yield requested.");
            scheduler_.yield();

            int after = scheduler_.get_task_id();

            if (after == before)
            {
                draw_log("Not enough quantum used. Waste more CPU time.");
            }
            else
            {
                draw_log("Switched to task " + std::to_string(after));
            }

            break;
        }

        case 'd':
            draw_log("Semaphore down requested.");
            resource1_.down(scheduler_.get_task_id());
            break;

        case 'u':
            draw_log("Semaphore up requested.");
            resource1_.up();
            break;

        case 'w':
            draw_log("Wasting CPU time...");
            waste_time(50);
            break;

        case 'k':
        {
            int current_task = scheduler_.get_task_id();

            draw_log("Kill task requested.");

            if (resource1_.get_lucky_task() == current_task)
            {
                draw_log("Current task owns resource. Releasing resource first.");
                resource1_.up();
            }

            scheduler_.kill();
            draw_log("Task killed.");
            break;
        }

        case 'g':
            draw_log("Garbage collection requested.");
            scheduler_.garbage();
            draw_log("Garbage collection complete.");
            break;

        case 'q':
            running_ = false;
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
        draw_all();
        int ch = wgetch(stdscr);
        handle_input(ch);
    }
}

// main()
int main()
{
    ULTIMA app;
    app.run();
    return 0;
}