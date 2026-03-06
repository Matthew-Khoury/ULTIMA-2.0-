#include <iostream>
#include <ncurses.h>

int main(int argc, char *argv[]) {
	initscr();            // Start curses mode
	printw("Hello, nCurses is working!");
	refresh();            // Print it onto the real screen
	getch();              // Wait for user input
	endwin();             // End curses mode

	return 0;
}
