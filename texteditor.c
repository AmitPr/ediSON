#include <ctype.h> //for keys and nonprintable chars;
#include <stdio.h>
#include <stdlib.h>
#include <termios.h> //for terminal attributes;
#include <unistd.h>

struct termios orig_termios;

void disableRawMode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); //sets terminal to its original attributes;
}

void enableRawMode()
{
    tcgetattr(STDIN_FILENO, &orig_termios); //store terminal attributes into this;

    atexit(disableRawMode); //when program exits, set to original;

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(ICRNL | IXON);                  //disables ctrl-s and ctrl-q; fixes ctrl-m; fix this for the future so we can quit using 'ctrl-q'
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); //modify struct; turn off canonical mode; disable ctrl-c, ctrl-z, ctrl-v;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);        //write new terminal attributes back out;
}

int main()
{
    enableRawMode();

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') //read in continuously or until user quits using 'q';
    {
        if (iscntrl(c))
        {
            printf("%d\n", c); //show nonprintable characters;
        }
        else
        {
            printf("%d ('%c')\n", c, c); //show keys;
        }
    }
    return 0;
}