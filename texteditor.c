#include <stdlib.h>
#include <termios.h> //for enableRawMode()
#include <unistd.h>

struct termios orig_termios;

void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); //sets terminal to its original attributes
}

void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios); //store terminal attributes into this

    atexit(disableRawMode); //when program exits, set to original

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO);                   //modify struct
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); //write new terminal attributes back out
}

int main()
{
    enableRawMode();

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q')
        ; //read in continuously or until user quits using 'q'
    return 0;
}