#include <panel.h>
#include <string.h>

void init_wins(WINDOW **wins);
void print_label(WINDOW **wins);

int main()
{   WINDOW *my_wins[2];
    PANEL  *my_panels[2];
    PANEL  *top;

    /* Initialize curses */
    initscr();
    start_color();
    //cbreak();

    /* Initialize all the colors */
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_BLUE, COLOR_BLACK);
    init_pair(4, COLOR_CYAN, COLOR_BLACK);

    init_wins(my_wins);
    
    /* Attach a panel to each window */ 
    my_panels[0] = new_panel(my_wins[0]);
    my_panels[1] = new_panel(my_wins[1]);

    /* Update the stacking order. */
    update_panels();

    /* Show it on the screen */
    attron(COLOR_PAIR(4));
    attroff(COLOR_PAIR(4));
    doupdate();
    refresh();

    /* Setup for chatting */
    wsetscrreg(my_wins[0], 0, LINES - 5);
    scrollok(my_wins[0], TRUE);
    while(1)
    {   
        wmove(my_wins[1], 1, 1);
        char st[99];
        wgetstr(my_wins[1], st);
        wprintw(my_wins[0], "%s\n", st);
        werase(my_wins[1]);
        print_label(my_wins);
        update_panels();
        doupdate();
    }
    endwin();
    return 0;
}

/* Put all the windows */
void init_wins(WINDOW **wins)
{
    char str[90];
    wins[0] = newwin(LINES - 4, COLS, 0, 0);
    wattron(wins[0], COLOR_PAIR(1));
    wprintw(wins[0], "Help: \n\":list\" \t\tList online users.\n\":logout\" \t\tLog out.\n\"@someone <message>\" \tSend a private message to someone.\n");
    wattroff(wins[0], COLOR_PAIR(1));
    
    wins[1] = newwin(4, COLS, LINES - 5, 0);
    box(wins[1], 0, 0);
    wattron(wins[1], COLOR_PAIR(2));

    strcpy(str, "Welcome to oubeichen's chatroom.Please enter a name.");
    mvwprintw(wins[1], 0, (COLS - strlen(str)) / 2, str);
    wattroff(wins[1], COLOR_PAIR(2));
    refresh();
}

void print_label(WINDOW **wins)
{
    box(wins[1], 0, 0);
    wattron(wins[1], COLOR_PAIR(2));
    mvwprintw(wins[1], 0, (COLS - 5) / 2, "Input");
    wattroff(wins[1], COLOR_PAIR(2));
}
