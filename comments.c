/*** includes ***/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h> //for keys and nonprintable chars;
#include <errno.h>//for program failure, errors'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h> //for terminal attributes;
#include <unistd.h>

/*** defines ***/
#define ediSON "0.0.0.1" //welcome msg;
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
  char *chars;
} erow;

struct editorConfig {
  int cx, cy;                  //cursor's x,y position;
  int rowoff;
  int coloff;
  int screenrows;              //terminal's # of rows
  int screencols;              //terminal's # of columns
  int numrows;
  erow *row;
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
void editorAppendRow(char *s, size_t len) {
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  int at = E.numrows;
  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';
  E.numrows++;
}

/*** file i/o ***/
void editorOpen(char *filename) {
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
  if (E.cy < E.rowoff) {
    E.rowoff = E.cy;
  }
  if (E.cy >= E.rowoff + E.screenrows) {
    E.rowoff = E.cy - E.screenrows + 1;
  }
  if (E.cx < E.coloff) {
    E.coloff = E.cx;
  }
  if (E.cx >= E.coloff + E.screencols) {
    E.coloff = E.cx - E.screencols + 1;
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
      int len = E.row[filerow].size - E.coloff;
      if (len < 0) {
        len = 0;
      }
      if (len > E.screencols) {
        len = E.screencols;
      }
      abAppend(ab, &E.row[filerow].chars[E.coloff], len);
    }
    abAppend(ab, "\x1b[K", 3); //clear each line as we redraw them; K erases part of current line, similarly to J, 2 erases whole, 1 erases left, 0 erases right of cursor;
    if (y < E.screenrows - 1) {
      abAppend(ab, "\r\n", 2);
    }
  }
}

void editorRefreshScreen() { //\x1b means escape and is always followed by [;
  editorScroll();

  struct abuf ab = ABUF_INIT; //create dynamic string;
  abAppend(&ab, "\x1b[?25l", 6); //l reset mode; hide cursor;
  abAppend(&ab, "\x1b[H", 3); //3 bytes long, H command position cursor to top left (1;1, we can do 20;24H if we want to maybe do nearer the middle);
  editorDrawRows(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, //specify specific coordinates
           (E.cx - E.coloff) + 1);                               //hide cursor;
  abAppend(&ab, buf, strlen(buf));

  /* abAppend(&ab, "\x1b[H", 3); */
  abAppend(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
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
  case END_KEY:
    E.cx = E.screencols - 1;
    break;

  case PAGE_UP:
  case PAGE_DOWN: {
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
  E.rowoff = 0;
  E.coloff = 0;
  E.numrows = 0;
  E.row = NULL;
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) {
    die("getWindowSize");
  }
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    editorOpen(argv[1]);
  }
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}
