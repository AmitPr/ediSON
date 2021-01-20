/*** includes ***/
#include <ctype.h> //for keys and nonprintable chars;
#include <errno.h> //for program failure, errors'
#include <stdio.h>
#include <stdlib.h>
#include <termios.h> //for terminal attributes;
#include <unistd.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/
struct termios orig_termios;

/*** terminal ***/
void die(const char *s) //anytime error, print descriptive msg;
{
    perror(s);
    exit(1);
}

void disableRawMode() //sets terminal to its original attributes (canonical mode);
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
    {
        die("tcsetattr");
    }
}

void enableRawMode() //rawmode (texteditor mode, unlike canonical mode);
{
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
    {
        die("tcgetattr");
    }

    tcgetattr(STDIN_FILENO, &orig_termios); //store terminal attributes into this;

    atexit(disableRawMode); //when program exits, set to original;

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); //disables ctrl-s and ctrl-q; fixes ctrl-m; turn off unnecessary flags; fix this for the future so we can quit using 'ctrl-q'
    raw.c_oflag &= ~(OPOST);                                  //turn off output processing features (carriage return);
    raw.c_cflag |= (CS8);                                     //turn off unnecessary flags
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);          //modify struct; turn off canonical mode; disable ctrl-c, ctrl-z, ctrl-v;
    raw.c_cc[VMIN] = 0;                                       //read() returns as soon as any input is put in
    raw.c_cc[VTIME] = 1;                                      //output prints 0 anytime there's no input
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)       //write new terminal attributes back out;
    {
        die("tcsetattr");
    }
}

/*** init ***/
int main()
{
    enableRawMode();

    while (1) //read in for some time
    {
        char c = '\0';
        if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN)
        {
            die("read");
        }
        if (iscntrl(c))
        {
            printf("%d\r\n", c); //show nonprintable characters;
        }
        else
        {
            printf("%d ('%c')\r\n", c, c); //show keys; use \r\n for newline now
        }
        if (c == CTRL_KEY('q')) //quit using 'q';
        {
            break;
        }
    }
    return 0;
}