#ifndef ULTIMA_H
#define ULTIMA_H

#include <string>
#include <ncurses.h>

class ULTIMA
{
public:
    ULTIMA();
    ~ULTIMA();

    void run();

private:
    WINDOW* heading_win_;
    WINDOW* task_win_;
    WINDOW* sema_win_;
    WINDOW* log_win_;
    WINDOW* console_win_;

    bool running_;

    // log window tracking
    int log_line_;

    void init_curses();
    void shutdown_curses();
    WINDOW* create_window(int height, int width, int y, int x);


    // TODO: Initialize Scheduler and create tasks
    void initialize_scheduler();

    // TODO: Display Scheduler task table
    void draw_tasks();

    // TODO: Display Semaphore information
    void draw_semaphore();

    // TODO: Display current scheduler state
    void draw_heading();

    void draw_log(const std::string& line);
    void draw_console();
    void draw_all();

    // TODO: Connect console commands to Scheduler/Semaphore
    void handle_input(int ch);
};

#endif