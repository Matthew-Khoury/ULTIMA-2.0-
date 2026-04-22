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
    WINDOW* memory_usage_win_;
    WINDOW* core_dump_win_;
    WINDOW* log_win_;
    WINDOW* console_win_;

    MCB mcb_;

    bool running_;
    bool paused_;

    int log_line_;
    int selected_mailbox_;
    int selected_semaphore_;

    bool show_core_dump_;

    void init_curses();
    void shutdown_curses();
    WINDOW* create_window(int height, int width, int y, int x);

    void initialize_scheduler();
    void draw_tasks();
    void draw_semaphore();
    void draw_heading();
    void draw_mailboxes();
    void draw_memory_usage();
    void draw_core_dump();

    void draw_log(const std::string& line);
    void draw_console();
    void draw_all();

    void handle_input(int ch);
    void waste_time(int factor);

    int find_task_for_handle(int handle) const;
    int active_memory_handle_count() const;
    void free_task_memory(tcb* task);
};

#endif