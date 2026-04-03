/* Code Framework built from Lab 6
Primary Author: Dylan Hurt*/
#ifndef ULTIMA_H
#define ULTIMA_H

#include <string>
#include <ncurses.h>
#include "MCB.h"

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
    WINDOW* mailbox_win_;
    WINDOW* log_win_;
    WINDOW* console_win_;

    MCB mcb_;

    bool running_;
    bool paused_;

    // log window tracking
    int log_line_;
    int selected_mailbox_;
    int selected_semaphore_;

    void init_curses();
    void shutdown_curses();
    WINDOW* create_window(int height, int width, int y, int x);

    void initialize_scheduler();
    void draw_tasks();
    void draw_semaphore();
    void draw_heading();
    void draw_mailboxes();

    void draw_log(const std::string& line);
    void draw_console();
    void draw_all();

    void handle_input(int ch);
    void waste_time(int factor);
};

#endif