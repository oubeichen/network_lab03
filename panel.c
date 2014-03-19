#include <panel.h>

void init_wins(WINDOW **wins);
void win_show(WINDOW *win, char *label, int label_color);
void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color);

int main()
{	WINDOW *my_wins[2];
	PANEL  *my_panels[2];
	PANEL  *top;
	int ch;

	/* Initialize curses */
	initscr();
	start_color();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);

	/* Initialize all the colors */
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_BLUE, COLOR_BLACK);
	init_pair(4, COLOR_CYAN, COLOR_BLACK);

	init_wins(my_wins);
	
	/* Attach a panel to each window */ 	/* Order is bottom up */
	my_panels[0] = new_panel(my_wins[0]); 	/* Push 0, order: stdscr-0 */
	my_panels[1] = new_panel(my_wins[1]); 	/* Push 1, order: stdscr-0-1 */

	/* Set up the user pointers to the next panel */
	set_panel_userptr(my_panels[0], my_panels[1]);
	set_panel_userptr(my_panels[1], my_panels[0]);

	/* Update the stacking order. 2nd panel will be on top */
	update_panels();

	/* Show it on the screen */
	attron(COLOR_PAIR(4));
	attroff(COLOR_PAIR(4));
	doupdate();

	wmove(my_wins[1],2,1);
	update_panels();
	doupdate();

	while((ch = getch()) != KEY_F(2))
	{	
		waddch(my_wins[1],ch);
		update_panels();
		doupdate();
	}
	endwin();
	return 0;
}

/* Put all the windows */
void init_wins(WINDOW **wins)
{	int x, y, i;
	char label[80];

	y = 0;
	x = 0;
	wins[0] = newwin(LINES - 5, COLS, y, x);
	sprintf(label, "Chat");
	win_show(wins[0], label, 1);
	y += LINES - 5;
	wins[1] = newwin(4, COLS, y, x);
	sprintf(label, "Input");
	win_show(wins[1], label, 2);
}

/* Show the window with a border and a label */
void win_show(WINDOW *win, char *label, int label_color)
{	int startx, starty, height, width;

	getbegyx(win, starty, startx);
	getmaxyx(win, height, width);

	box(win, 0, 0);
	
	print_in_middle(win, 1, 0, width, label, COLOR_PAIR(label_color));
}

void print_in_middle(WINDOW *win, int starty, int startx, int width, char *string, chtype color)
{	int length, x, y;
	float temp;

	if(win == NULL)
		win = stdscr;
	getyx(win, y, x);
	if(startx != 0)
		x = startx;
	if(starty != 0)
		y = starty;
	if(width == 0)
		width = 80;

	length = strlen(string);
	temp = (width - length)/ 2;
	x = startx + (int)temp;
	wattron(win, color);
	mvwprintw(win, y - 1, x, "%s", string);
	wattroff(win, color);
	refresh();
}
