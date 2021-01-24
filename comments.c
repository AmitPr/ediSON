/*** includes ***/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h> //for keys and nonprintable chars;
#include <errno.h>//for program failure, errors'
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h> //for terminal attributes;
#include <time.h> //add timestamps for msgs;
#include <unistd.h>

/*** defines ***/
#define ediSON "0.0.0.1" //welcome msg;
#define KILO_TAB_STOP 8 //make length of a tab constant;
#define CTRL_KEY(k) ((k)&0x1f) //ctrl + q to exit;

enum editorKey { //use arrow keys to move cursor;
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

/*** data ***/
typedef struct erow {
  int size;
  int rsize; //contain size of the contents of render
  char *chars;
  char *render; //contain the actual chars to draw
} erow;

struct editorConfig {
  int cx, cy;                  //cursor's x,y position;
  int rx;                      //index into the render field; not all chars have same width, this compensates for it;
  int rowoff;
  int coloff;
  int screenrows;              //terminal's # of rows
  int screencols;              //terminal's # of columns
  int numrows;
  erow *row;
  char *filename;              //save a copy of the filename here when a file opens;
  char statusmsg[80];          //msgs that we ask the user;
  time_t statusmsg_time;       //timestamp for the msg;
  struct termios orignal_termious; //all of the canonical v. raw mode
};
struct editorConfig E;

/*** terminal ***/
void die(const char *s) { //anytime error, print descriptive msg;
  write(STDOUT_FILENO, "\x1b[2J", 4); //clear screen and reposition cursor when program exits
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void disableRawMode() { //sets terminal to its original attributes (canonical mode);
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orignal_termious) == -1) {
    die("tcgetattr");
  }
}

void enableRawMode() { //rawmode (texteditor mode, unlike canonical mode);
  if (tcgetattr(STDIN_FILENO, &E.orignal_termious) == -1) {
    die("tcgetattr");
  }
  atexit(disableRawMode); //when program exits, set to original;

  struct termios raw = E.orignal_termious;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); //disables ctrl-s and ctrl-q; fixes ctrl-m; turn off unnecessary flags;
  raw.c_oflag &= ~(OPOST);                                  //turn off output processing features (carriage return);
  raw.c_cflag |= ~(CS8);                                     //turn off unnecessary flags
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);           //modify struct; turn off canonical mode; disable ctrl-c, ctrl-z, ctrl-v;
  raw.c_cc[VMIN] = 0;                                       //read() returns as soon as any input is put in
  raw.c_cc[VTIME] = 1;                                      //output prints 0 anytime there's no input

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    die("tcgetattr");
  }
}
int editorReadKey() { //wait for one keypress, return it;
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  if (c == '\x1b') { //replace WASD with arrow keys
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';
    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1)
          return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
          case '1':
            return HOME_KEY;
          case '3':
            return DEL_KEY;
          case '4':
            return END_KEY;
          case '5':
            return PAGE_UP;
          case '6':
            return PAGE_DOWN;
          case '7':
            return HOME_KEY;
          case '8':
            return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
        case 'A':
          return ARROW_UP;
        case 'B':
          return ARROW_DOWN;
        case 'C':
          return ARROW_RIGHT;
        case 'D':
          return ARROW_LEFT;
        case 'H':
          return HOME_KEY;
        case 'F':
          return END_KEY;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
      case 'H':
        return HOME_KEY;
      case 'F':
        return END_KEY;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}

int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;

  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
    return -1;
  }

  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) {
      break;
    }
    if (buf[i] == 'R') {
      break;
    }
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[') {
    return -1;
  }

  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
    return -1;
  }

  return 0;
}

int getWindowSize(int *rows, int *cols) { //find width, height of terminal
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) { //C move cursor right, B move cursor down, by 999 times to guarantee cursor at bottom right;
      return -1;
    }
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** row operations ***/
int editorRowCxToRx(erow *row, int cx) { //calculate value of E.rx; converts chars index into render index
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (KILO_TAB_STOP - 1) - (rx % KILO_TAB_STOP);
    rx++;
  }
  return rx;
}

void editorUpdateRow(erow *row) { //use string of erow to fill contents of render, copy each char from char to render;
  int tabs = 0;
  int j;
  for (j = 0; j < row->size; j++) { //count tabs to know much memory to allocate for render;
    if (row->chars[j] == '\t') tabs++;
  }
  free(row->render);
  row->render = malloc(row->size + tabs*(KILO_TAB_STOP - 1) + 1);;
  int idx = 0;
  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % KILO_TAB_STOP != 0) {
        row->render[idx++] = ' ';
      }
    } else {
        row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = idx;
}

void editorAppendRow(char *s, size_t len) {
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  int at = E.numrows;
  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  editorUpdateRow(&E.row[at]);
  E.numrows++;
}

/*** file i/o ***/
void editorOpen(char *filename) {
  free(E.filename);
  E.filename = strdup(filename); //make copy of given string, allocating memory and assuming we free memory;
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    die("fopen");
  }
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 &&
           (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {

      linelen--;
    }
    editorAppendRow(line, linelen);
  }
  free(line);
  fclose(fp);
}

/*** append buffer ***/
struct abuf { //make our own dynamic string type
  char *b;
  int len;
};

#define ABUF_INIT                                                              \
  { NULL, 0 }

void abAppend(struct abuf *ab, const char *s, int len) { //add to dynamic string;
  char *new = realloc(ab->b, ab->len + len); //extend current size of memory, or free and allocate a new memory block;
  if (new == NULL)
    return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) { free(ab->b); } //free dynamic string;

/*** output ***/
void editorScroll() {
  E.rx = 0;
  if (E.cy < E.numrows) { //set E.rx to correct value;
    E.rx = editorRowCxToRx(&E.row[E.cy], E.cx);
  }
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  if (E.rx < E.coloff) {
    E.coloff = E.rx;
  }
  if (E.rx >= E.coloff + E.screencols) {
    E.coloff = E.rx - E.screencols + 1;
  }
}

void editorDrawRows(struct abuf *ab) { //draws tilde on each row;
  int y;
  for (y = 0; y < E.screenrows; y++) { //display correct # of tildes;
    int filerow = y + E.rowoff;
    if (filerow >= E.numrows) { //display welcome msg;
      if (y >= E.numrows) {
        if (E.numrows == 0 && y == E.screenrows / 3) {
          char welcome[80];
          int welcomelen = snprintf(welcome, sizeof(welcome),
                                    "ediSON -- version %s", ediSON);
          if (welcomelen > E.screencols)
            welcomelen = E.screencols;
          int padding = (E.screencols - welcomelen) / 2;
          if (padding) {
            abAppend(ab, "~", 1);
            padding--;
          }
          while (padding--)
            abAppend(ab, " ", 1);
          abAppend(ab, welcome, welcomelen);
        } else {
          abAppend(ab, "~", 1);
        }
      }
    } else {
      int len = E.row[filerow].rsize - E.coloff; 
      if (len < 0) {
        len = 0;
      }
      if (len > E.screencols) {
        abAppend(ab, &E.row[filerow].render[E.coloff], len);
      }
      abAppend(ab, &E.row[filerow].chars[E.coloff], len);
    }
    abAppend(ab, "\x1b[K", 3); //clear each line as we redraw them; K erases part of current line, similarly to J, 2 erases whole, 1 erases left, 0 erases right of cursor;
    abAppend(ab, "\r\n", 2);
  }
}

void editorDrawStatusBar(struct abuf *ab) { //make inverted background at the last line; \x1b[7m inverts background colors, which in this case is white; /x1b[m makes it back to normal;
  abAppend(ab, "\x1b[7m", 4);               ///m adds graphics so 1 is bold, 4 is underscore, 5 blink, 7 inverted colors;
  char status[80], rstatus[80];
  int len = snprintf(status, sizeof(status), "%.20s - %d lines",
  E.filename ? E.filename : "[No Name]", E.numrows); //if no file was given, we set statusbar to no name;
  int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
  E.cy + 1, E.numrows);
  if (len > E.screencols) {
    len = E.screencols;
  }
  abAppend(ab, status, len);
  while (len < E.screencols) {
    if (E.screencols - len == rlen) { //show current line number on the right;
      abAppend(ab, rstatus, rlen);
      break;
    } else {
    abAppend(ab, " ", 1);
    len++;
    }
  }
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2); //make another line under status bar;
}

void editorDrawMessageBar(struct abuf *ab) { //create the msg bar;
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(E.statusmsg);
  if (msglen > E.screencols) msglen = E.screencols;
  if (msglen && time(NULL) - E.statusmsg_time < 5)
    abAppend(ab, E.statusmsg, msglen);
}

void editorRefreshScreen() { //\x1b means escape and is always followed by [;
  editorScroll();

  struct abuf ab = ABUF_INIT; //create dynamic string;
  abAppend(&ab, "\x1b[?25l", 6); //l reset mode; hide cursor;
  abAppend(&ab, "\x1b[H", 3); //3 bytes long, H command position cursor to top left (1;1, we can do 20;24H if we want to maybe do nearer the middle);
  editorDrawRows(&ab);
  editorDrawStatusBar(&ab); //add the status bar;
  editorDrawMessageBar(&ab); //add the msg bar;

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, //specify specific coordinates;
           (E.rx - E.coloff) + 1);                               //hide cursor;
  abAppend(&ab, buf, strlen(buf));

  /* abAppend(&ab, "\x1b[H", 3); */
  abAppend(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(const char *fmt, ...) { //take a format string and variables; ... makes this a variadic function, meaning it can take any # of args;
  va_list ap;
  va_start(ap, fmt); //fmt is passed so address of all args are known;
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap); //store string in Estatusmsg;
  va_end(ap);
  E.statusmsg_time = time(NULL); //set estatusmsg_time to current time;
}

/*** input ***/
void editorMoveCursor(int key) { //move cursor with WASD
  erow *row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  switch (key) {
  case ARROW_LEFT:
    if (E.cx != 0) {
      E.cx--;
    } else if (E.cy > 0) {
      E.cy--;
      E.cx = E.row[E.cy].size;
    }
    break;
  case ARROW_RIGHT:
    if (row && E.cx < row->size) {
      E.cx++;
    } else if (row && E.cx == row->size) {
      E.cy++;
      E.cx = 0;
    }
    break;
  case ARROW_UP:
    if (E.cy != 0) {
      E.cy--;
    }
    break;
  case ARROW_DOWN:
    if (E.cy < E.numrows) {
      E.cy++;
    }
    break;
  }
  row = (E.cy >= E.numrows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->size : 0;
  if (E.cx > rowlen) {
    E.cx = rowlen;
  }
}

void editorProcessKeypress() { //wait for keypress, handle it;
  int c = editorReadKey();
  switch (c) {
  case CTRL_KEY('q'):
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;

  case HOME_KEY:
    E.cx = 0;
    break;
  case END_KEY: //end of the line
    if (E.cy < E.numrows) { //make end key work;
      E.cx = E.row[E.cy].size;
    }
    break;

  case PAGE_UP:
  case PAGE_DOWN: { //end of page;
    if (c == PAGE_UP) { //make pageup and pagedown work;
      E.cy = E.rowoff;
    } else if (c == PAGE_DOWN) {
        E.cy = E.rowoff + E.screenrows - 1;
        if (E.cy > E.numrows) {
          E.cy = E.numrows;
        }
    }
    int times = E.screenrows;
    while (times--)
      editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
  } break;

  case ARROW_UP:
  case ARROW_DOWN:
  case ARROW_LEFT:
  case ARROW_RIGHT:
    editorMoveCursor(c);
    break;
  }
}

/*** init ***/
void initEditor() {
  E.cx = 0;
  E.cy = 0;
  E.rx = 0;
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  E.filename = NULL; //set to null so if there's no file given;
  E.statusmsg[0] = '\0'; //empty so there's no default msg;
  E.statusmsg_time = 0;
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
    die("getWindowSize");
  }
  E.screenrows -= 2; //make two lines of space at the end of the screen;
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  }
  editorSetStatusMessage("HELP: Ctrl-Q = quit"); //help msg with quit keybinds;
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}
