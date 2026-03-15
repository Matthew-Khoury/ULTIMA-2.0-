#include "ULTIMA.h"

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
      running_(true),
      log_line_(1)
{
    // TODO: create scheduler and initialize tasks
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

    heading_win_ = create_window(5, 96, 1, 2);
    task_win_    = create_window(12, 52, 7, 2);
    sema_win_    = create_window(12, 52, 7, 56);
    log_win_     = create_window(12, 72, 20, 2);
    console_win_ = create_window(12, 34, 20, 75);

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

    // TODO: initialize scheduler and create tasks
}

// draw_heading()
// Draws the heading window
void ULTIMA::draw_heading()
{
    werase(heading_win_);
    box(heading_win_, 0, 0);

    mvwprintw(heading_win_, 1, 2, "ULTIMA Project");

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

    // TODO: show scheduler process table

    wrefresh(task_win_);
}

// draw_semaphore()
// Draws the semaphore window
void ULTIMA::draw_semaphore()
{
    werase(sema_win_);
    box(sema_win_, 0, 0);

    mvwprintw(sema_win_, 1, 2, "Semaphore Dump:");
    mvwprintw(sema_win_, 3, 2, "Resource:");
    mvwprintw(sema_win_, 4, 2, "Sema_value:");
    mvwprintw(sema_win_, 5, 2, "Sema_queue:");

    // TODO: show semaphore state

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
    mvwprintw(console_win_, 6, 2, "k : Kill current task");
    mvwprintw(console_win_, 7, 2, "g : Garbage collect");
    mvwprintw(console_win_, 8, 2, "q : Quit");

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

// handle_input()
// Handles keyboard inputs from the console
void ULTIMA::handle_input(int ch)
{
    switch (ch)
    {
        case 'n':
            draw_log("Task yield requested.");
            // TODO: scheduler yield function
            break;

        case 'd':
            draw_log("Semaphore down requested.");
            // TODO: semaphore request resource
            break;

        case 'u':
            draw_log("Semaphore up requested.");
            // TODO: semaphore release resource
            break;

        case 'k':
            draw_log("Kill task requested.");
            // TODO: scheduler kill task
            break;

        case 'g':
            draw_log("Garbage collection requested.");
            // TODO: scheduler garbage collection
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