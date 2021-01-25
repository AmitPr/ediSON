/*** includes ***/
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include "JSONTokenizer.h"
#include "utils.h"
#include <ctype.h> //for keys and nonprintable chars;
#include <errno.h> //for program failure, errors'
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h> //for terminal attributes;
#include <time.h>    //add timestamps for msgs;
#include <unistd.h>

/*** defines ***/
#define ediSON "0.0.0.1"       // welcome msg version;
#define ediSON_TAB_STOP 4      // make length of length of a tab constant;
#define ediSON_QUIT_TIMES 1    // How many times the quit key must be pressed;
#define CTRL_KEY(k) ((k)&0x1f) // Macro fro ctrl with another key;

int errorLoc = -1;

enum editorKey { // use arrow keys to move cursor
  BACKSPACE = 127,
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

enum editorHighlight {
  HL_NORMAL = 0,
  HL_KEYWORD1,
  HL_KEYWORD2,
  HL_STRING,
  HL_NUMBER,
  HL_MATCH,
  HL_KEY
};

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

/*** data ***/
struct editorSyntax {
  char *filetype;
  char **filematch;
  char **keywords;
  int flags;
};

typedef struct erow {
  int size;
  int rsize; // contain size of the contents of render
  char *chars;
  char *render; // contain the actual chars to draw
  unsigned char *hl;
} erow;

struct editorConfig {
  int cx, cy; // cursor's x,y position;
  int rx; // index into the render field; not all chars have same width, this
          // compensates for it;
  int rowoff;
  int coloff;
  int screenrows; // terminal's # of rows
  int screencols; // terminal's # of columns
  int raw_screenrows;
  int raw_screencols;
  int numrows;
  erow *row;
  int dirty;
  int linenum_indent;
  char *filename;        // save a copy of the filename here when a file opens;
  char statusmsg[80];    // msgs that we ask the user;
  time_t statusmsg_time; // timestamp for the msg;
  struct editorSyntax *syntax;
  struct termios orignal_termious; // all of the canonical v. raw mode
};

struct editorConfig E;

/*** filetypes ***/
char *json_extensions[] = {".json", ".js", ".ts", NULL};

char *JSON_keywords[] = {"true", "false", "null|", "NULL|", NULL};

struct editorSyntax HLDB[] = {
    {"json", json_extensions, JSON_keywords,
     HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
};

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

/** prototypes ***/
void editorSetStatusMessage(const char *fmt, ...);
void editorRefreshScreen();
char *editorPrompt(char *prompt, void (*callback)(char *, int));

/*** terminal ***/
void die(const char *s) { // anytime error, print descriptive msg;
  write(STDOUT_FILENO, "\x1b[2J",
        4); // clear screen and reposition cursor when program exits
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void disableRawMode() { // sets terminal to its original attributes (canonical
                        // mode);
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orignal_termious) == -1) {
    die("tcgetattr");
  }
}

void enableRawMode() { // rawmode (texteditor mode, unlike canonical mode);
  if (tcgetattr(STDIN_FILENO, &E.orignal_termious) == -1) {
    die("tcgetattr");
  }
  atexit(disableRawMode); // when program exits, set to original;

  struct termios raw = E.orignal_termious;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP |
                   IXON); // disables ctrl-s and ctrl-q; fixes ctrl-m; turn
                          // off unnecessary flags;
  raw.c_oflag &=
      ~(OPOST); // turn off output processing features (carriage return);
  raw.c_cflag |= ~(CS8); // turn off unnecessary flags
  raw.c_lflag &=
      ~(ECHO | ICANON | IEXTEN | ISIG); // modify struct; turn off canonical
                                        // mode; disable ctrl-c, ctrl-z, ctrl-v;
  raw.c_cc[VMIN] = 0;  // read() returns as soon as any input is put in
  raw.c_cc[VTIME] = 1; // output prints 0 anytime there's no input

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    die("tcgetattr");
  }
}

int editorReadKey() { // wait for one keypress, return it;
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  if (c == '\x1b') { // replace WASD with arrow keys
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

int getWindowSize(int *rows, int *cols) { // find width, height of terminal
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) !=
        12) { // C move cursor right, B move cursor down, by 999 times to
              // guarantee cursor at bottom right;
      return -1;
    }
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/*** syntax highlighting ***/
int is_separator(int c) {
  return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void editorUpdateSyntax(erow *row) {
  row->hl = realloc(row->hl, row->rsize);
  memset(row->hl, HL_NORMAL, row->rsize);
  if (E.syntax == NULL) {
    return;
  }

  char **keywords = E.syntax->keywords;

  int prev_sep = 1;
  int in_string = 0;
  int i = 0;
  int string_type = HL_STRING;

  while (i < row->rsize) {
    char c = row->render[i];
    unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
    if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
      if (in_string) {
        row->hl[i] = string_type;

        if (c == '\\' && i + 1 < row->rsize) {
          row->hl[i + 1] = string_type;
          i += 2;
          continue;
        }

        if (c == in_string)
          in_string = 0;
        i++;
        prev_sep = 1;
        continue;
      } else {
        if ((c == '"' || c == '\'')) {
          int j = i + 1;
          int in_string_tmp = c;
          string_type = HL_STRING;
          while (j < row->rsize) {
            char cc = row->render[j];
            if ((cc == '"' || cc == '\'')) {
              if (!in_string_tmp) {
                break;
              } else {
                in_string_tmp = 0;
              }
            }
            if (cc == ':' && !in_string_tmp) {
              string_type = HL_KEY;
              break;
            }
            j++;
          }
          row->hl[i] = string_type;
          in_string = c;
          i++;
          continue;
        }
      }
    }

    if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
      if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
          (c == '.' && prev_hl == HL_NUMBER)) {
        row->hl[i] = HL_NUMBER;
        i++;
        prev_sep = 0;
        continue;
      }
    }

    if (prev_sep) {
      int j;
      for (j = 0; keywords[j]; j++) {
        int klen = strlen(keywords[j]);
        int kw2 = keywords[j][klen - 1] == '|';
        if (kw2)
          klen--;
        if (!strncmp(&row->render[i], keywords[j], klen) &&
            is_separator(row->render[i + klen])) {
          memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
          i += klen;
          break;
        }
      }
      if (keywords[j] != NULL) {
        prev_sep = 0;
        continue;
      }
    }

    prev_sep = is_separator(c);
    i++;
  }
}

int editorSyntaxToColor(int hl, int checkBold) {
  switch (hl) {
  case HL_KEYWORD1:
    if (checkBold)
      return 1;
    return 34;
  case HL_KEYWORD2:
    return 35;
  case HL_STRING:
    return 32;
  case HL_NUMBER:
    return 33;
  case HL_MATCH:
    return 36;
  case HL_KEY:
    if (checkBold)
      return 1;
    return 37;
  default:
    return 37;
  }
}

void editorSelectSyntaxHighlight() {
  E.syntax = NULL;
  if (E.filename == NULL)
    return;
  char *ext = strrchr(E.filename, '.');
  for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
    struct editorSyntax *s = &HLDB[j];
    unsigned int i = 0;
    while (s->filematch[i]) {
      int is_ext = (s->filematch[i][0] == '.');
      if ((is_ext && ext && !strcmp(ext, s->filematch[i])) ||
          (!is_ext && strstr(E.filename, s->filematch[i]))) {
        E.syntax = s;

        int filerow;
        for (filerow = 0; filerow < E.numrows; filerow++) {
          editorUpdateSyntax(&E.row[filerow]);
        }

        return;
      }
      i++;
    }
  }
}

/*** row operations ***/
int editorRowCxToRx(erow *row, int cx) { // calculate value of E.rx; converts
                                         // chars index into render index
  int rx = 0;
  int j;
  for (j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (ediSON_TAB_STOP - 1) - (rx % ediSON_TAB_STOP);
    rx++;
  }
  return rx;
}

int editorRowRxToCx(erow *row,
                    int rx) { // use string of erow to fill contents of render,
                              // copy each char from char to render;
  int cur_rx = 0;
  int cx;
  for (cx = 0; cx < row->size;
       cx++) { // count tabs to know much memory to allocate for render;
    if (row->chars[cx] == '\t')
      cur_rx += (ediSON_TAB_STOP - 1) - (cur_rx % ediSON_TAB_STOP);
    cur_rx++;
    if (cur_rx > rx)
      return cx;
  }
  return cx;
}

void editorUpdateRow(erow *row) {
  int tabs = 0;
  int j;
  for (j = 0; j < row->size; j++)
    if (row->chars[j] == '\t')
      tabs++;
  free(row->render);
  row->render = malloc(row->size + tabs * (ediSON_TAB_STOP - 1) + 1);
  int idx = 0;
  for (j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % ediSON_TAB_STOP != 0) {
        row->render[idx++] = ' ';
      }
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->rsize = idx;
  editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
  if (at < 0 || at > E.numrows) {
    return;
  }
  E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
  memmove(&E.row[at + 1], &E.row[at], sizeof(erow) * (E.numrows - at));

  E.row[at].size = len;
  E.row[at].chars = malloc(len + 1);
  memcpy(E.row[at].chars, s, len);
  E.row[at].chars[len] = '\0';

  E.row[at].rsize = 0;
  E.row[at].render = NULL;
  E.row[at].hl = NULL;
  editorUpdateRow(&E.row[at]);

  E.numrows++;
  E.dirty++;
}

void editorFreeRow(erow *row) {
  free(row->render);
  free(row->chars);
  free(row->hl);
}

void editorDelRow(int at) {
  if (at < 0 || at >= E.numrows) {
    return;
  }
  editorFreeRow(&E.row[at]);
  memmove(&E.row[at], &E.row[at + 1], sizeof(erow) * (E.numrows - at - 1));
  E.numrows--;
  E.dirty++;
}

void editorRowInsertChar(erow *row, int at, int c) {
  if (at < 0 || at > row->size) {
    at = row->size;
  }
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editorUpdateRow(row);
  E.dirty++;
}

void editorInsertNewline() {
  if (E.cx == 0) {
    editorInsertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    editorInsertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    editorUpdateRow(row);
  }
  E.cy++;
  E.cx = 0;
}

void editorRowDelChar(erow *row, int at) {
  if (at < 0 || at >= row->size) {
    return;
  }
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  editorUpdateRow(row);
  E.dirty++;
}

void editorRowAppendString(erow *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editorUpdateRow(row);
  E.dirty++;
}

/*** editor operations ***/
void editorInsertChar(int c) {
  if (E.cy == E.numrows) {
    editorInsertRow(E.numrows, "", 0);
  }
  editorRowInsertChar(&E.row[E.cy], E.cx, c);
  E.cx++;
}

void editorDelChar() {
  if (E.cy == E.numrows) {
    return;
  }
  if (E.cx == 0 && E.cy == 0) {
    return;
  }
  erow *row = &E.row[E.cy];
  if (E.cx > 0) {
    editorRowDelChar(row, E.cx - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    editorRowAppendString(&E.row[E.cy - 1], row->chars, row->size);
    editorDelRow(E.cy);
    E.cy--;
  }
}

/*** file i/o ***/

char *editorRowsToString(int *buflen) {
  int totlen = 0;
  int j;
  for (j = 0; j < E.numrows; j++) {
    totlen += E.row[j].size + 1;
  }
  *buflen = totlen;
  char *buf = malloc(totlen);
  char *p = buf;
  for (j = 0; j < E.numrows; j++) {
    memcpy(p, E.row[j].chars, E.row[j].size);
    p += E.row[j].size;
    *p = '\n';
    p++;
  }
  return buf;
}

void editorOpenJSON(char *filename) {
  free(E.filename);
  E.filename = strdup(filename); // make copy of given string, allocating
                                 // memory and assuming we free memory;
  editorSelectSyntaxHighlight();
  FILE *fp = fopen(filename, "r");
  if (!fp) {
    die("fopen");
  }
  char *buf = NULL; // A string we can read the entire file into
  long length;
  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  buf = (char *)malloc(length + 1);
  if (buf) {
    if (!fread(buf, 1, length, fp)) {
      printf("Error reading in file.\n");
      return;
    }
  } else {
    printf("Error allocating memory for file buffer\n");
    return;
  }
  fclose(fp);
  buf[length] = '\0';
  char *buf_cpy = buf;
  struct JSONToken *tokenized = parseJSON(&buf_cpy);
  char *formatted;
  if (!tokenized) {
    formatted = buf;
  } else {
    formatted = getFormattedString(tokenized, &buf);
  }
  char *pos = NULL;
  pos = strtok(formatted, "\n");
  while (pos != NULL) {
    editorInsertRow(E.numrows, pos, strlen(pos));
    pos = strtok(NULL, "\n");
  }
  if (tokenized) {
    freeJSON(tokenized);
    free(formatted);
  }
  free(buf);
  E.dirty = 0;
}

void editorOpen(char *filename) {
  free(E.filename);
  E.filename = strdup(filename); // make copy of given string, allocating
                                 // memory and assuming we free memory;

  editorSelectSyntaxHighlight();
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
    editorInsertRow(E.numrows, line, linelen);
  }
  free(line);
  fclose(fp);
  E.dirty = 0;
}

void editorSave() {
  if (E.filename == NULL) {
    E.filename = editorPrompt("Save as: %s (ESC to cancel)", NULL);
    if (E.filename == NULL) {
      editorSetStatusMessage("Save aborted");
      return;
    }
    editorSelectSyntaxHighlight();
  }

  int len;
  char *buf = editorRowsToString(&len);
  int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
  if (fd != -1) {
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        E.dirty = 0;
        editorSetStatusMessage("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }
  free(buf);
  editorSetStatusMessage("Can't save! I/O error: %s", strerror(errno));
}

/*** find ***/

void editorFindCallback(char *query, int key) {
  static int last_match = -1;
  static int direction = 1;

  static int saved_hl_line;
  static char *saved_hl = NULL;
  if (saved_hl) {
    memcpy(E.row[saved_hl_line].hl, saved_hl, E.row[saved_hl_line].rsize);
    free(saved_hl);
    saved_hl = NULL;
  }

  if (key == '\r' || key == '\x1b') {
    last_match = -1;
    direction = 1;
    return;
  } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
    direction = 1;
  } else if (key == ARROW_LEFT || key == ARROW_UP) {
    direction = -1;
  } else {
    last_match = -1;
    direction = 1;
  }

  if (last_match == -1) {
    direction = 1;
  }
  int current = last_match;
  int i;
  for (i = 0; i < E.numrows; i++) {
    current += direction;
    if (current == -1) {
      current = E.numrows - 1;
    } else if (current == E.numrows) {
      current = 0;
    }
    erow *row = &E.row[current];
    char *match = strstr(row->render, query);
    if (match) {
      last_match = current;
      E.cy = current;
      E.cx = editorRowRxToCx(row, match - row->render);
      E.rowoff = E.numrows;

      saved_hl_line = current;
      saved_hl = malloc(row->rsize);
      memcpy(saved_hl, row->hl, row->rsize);
      memset(&row->hl[match - row->render], HL_MATCH, strlen(query));
      break;
    }
  }
}

void editorFind() {
  int saved_cx = E.cx;
  int saved_cy = E.cy;
  int saved_coloff = E.coloff;
  int saved_rowoff = E.rowoff;
  char *query =
      editorPrompt("Search: %s (Use ESC/Arrows/Enter)", editorFindCallback);
  if (query) {
    free(query);
  } else {
    E.cx = saved_cx;
    E.cy = saved_cy;
    E.coloff = saved_coloff;
    E.rowoff = saved_rowoff;
  }
}

/*** append buffer ***/
struct abuf { // make our own dynamic string type
  char *b;
  int len;
};

#define ABUF_INIT                                                              \
  { NULL, 0 }

void abAppend(struct abuf *ab, const char *s,
              int len) { // add to dynamic string;
  char *new =
      realloc(ab->b, ab->len + len); // extend current size of memory, or
                                     // free and allocate a new memory block;
  if (new == NULL)
    return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab) { free(ab->b); } // free dynamic string;

/*** output ***/
void editorScroll() {
  E.rx = 0;
  if (E.cy < E.numrows) { // set E.rx to correct value;
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

void editorDrawRows(
    struct abuf *ab) { // Draws each row, draws tilde on each empty row.
  int y;
  int current_position = 0;
  for (y = 0; y < E.screenrows; y++) { // display correct # of tildes;
    int filerow = y + E.rowoff;

    // draw line
    char format[8];
    char linenum[E.linenum_indent + 1];
    memset(linenum, ' ', E.linenum_indent);
    snprintf(format, 5, "%%%dd ", E.linenum_indent - 1);
    if (filerow < E.numrows) {
      snprintf(linenum, E.linenum_indent + 1, format, filerow + 1);
    }
    abAppend(ab, linenum, E.linenum_indent);

    if (filerow >= E.numrows) { // display welcome msg;
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
      if (len < 0)
        len = 0;
      if (len > E.screencols)
        len = E.screencols;
      char *c = &E.row[filerow].render[E.coloff];
      unsigned char *hl = &E.row[filerow].hl[E.coloff];
      int current_color = -1;
      int j;
      for (j = 0; j < len; j++) {
        if (current_position == errorLoc) {
          abAppend(ab, "\x1b[41m", 5);
        } else if (current_position - 1 == errorLoc) {
          abAppend(ab, "\x1b[0m", 4);
        }
        if (iscntrl(c[j])) {
          char sym = (c[j] <= 26) ? '@' + c[j] : '?';
          abAppend(ab, "\x1b[7m", 4);
          abAppend(ab, &sym, 1);
          abAppend(ab, "\x1b[m", 3);
          if (current_color != -1) {
            char buf[16];
            int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
            abAppend(ab, buf, clen);
          }
        } else if (hl[j] == HL_NORMAL) {
          if (current_color != -1) {
            abAppend(ab, "\x1b[39m", 5);
            current_color = -1;
          }
          abAppend(ab, &c[j], 1);
        } else {
          int color = editorSyntaxToColor(hl[j], 0);
          if (color != current_color) {
            current_color = color;
            char buf[16];
            int clen;
            if (editorSyntaxToColor(hl[j], 1)) {
              clen = snprintf(buf, sizeof(buf), "\x1b[%d;1m", color);
            } else {
              clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
            }
            abAppend(ab, buf, clen);
          }
          abAppend(ab, &c[j], 1);
        }
        current_position++;
      }
      abAppend(ab, "\x1b[39m", 5);
    }
    abAppend(ab, "\x1b[K",
             3); // clear each line as we redraw them; K erases part of
                 // current line, similarly to J, 2 erases whole, 1 erases
                 // left, 0 erases right of cursor;
    abAppend(ab, "\r\n", 2);
  }
}

void editorDrawStatusBar(
    struct abuf *ab) { // make inverted background at the last line; \x1b[7m
                       // inverts background colors, which in this case is
                       // white; /x1b[m makes it back to normal;
  abAppend(ab, "\x1b[7m", 4); /// m adds graphics so 1 is bold, 4 is
                              /// underscore, 5 blink, 7 inverted colors;
  char status[80], rstatus[80];
  int len = snprintf(
      status, sizeof(status), "%.20s - %d lines %s",
      E.filename ? E.filename : "[No Name]", E.numrows,
      E.dirty ? "(modified)"
              : ""); // if no file was given, we set statusbar to no name;
  int rlen =
      snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
               E.syntax ? E.syntax->filetype : "no ft", E.cy + 1, E.numrows);
  if (len > E.screencols) {
    len = E.screencols;
  }
  abAppend(ab, status, len);
  while (len < E.screencols) {
    if (E.screencols - len == rlen) { // show current line number on the right;
      abAppend(ab, rstatus, rlen);
      break;
    } else {
      abAppend(ab, " ", 1);
      len++;
    }
  }
  abAppend(ab, "\x1b[m", 3);
  abAppend(ab, "\r\n", 2); // make another line under status bar;
}

void editorDrawMessageBar(struct abuf *ab) { // create the msg bar;
  abAppend(ab, "\x1b[K", 3);
  int msglen = strlen(E.statusmsg);
  if (msglen > E.screencols)
    msglen = E.screencols;
  if (msglen && time(NULL) - E.statusmsg_time < 5)
    abAppend(ab, E.statusmsg, msglen);
}

void editorUpdateLinenumIndent() {
  int digit;
  int numrows = E.numrows;

  if (numrows == 0) {
    digit = 0;
    E.linenum_indent = 2;
    return;
  }

  digit = 1;
  while (numrows >= 10) {
    numrows = numrows / 10;
    digit++;
  }
  E.linenum_indent = digit + 2;
}

void editorRefreshScreen() { //\x1b means escape and is always followed by [;
  editorUpdateLinenumIndent();
  E.screencols = E.raw_screencols - E.linenum_indent;
  editorScroll();

  struct abuf ab = ABUF_INIT;    // create dynamic string;
  abAppend(&ab, "\x1b[?25l", 6); // l reset mode; hide cursor;
  abAppend(&ab, "\x1b[H",
           3); // 3 bytes long, H command position cursor to top left (1;1,
               // we can do 20;24H if we want to maybe do nearer the middle);

  editorDrawRows(&ab);
  editorDrawStatusBar(&ab);  // add the status bar;
  editorDrawMessageBar(&ab); // add the msg bar;

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
           E.cy - E.rowoff + 1, // specify specific coordinates;
           E.rx - E.coloff + 1 + E.linenum_indent); // hide cursor;
  abAppend(&ab, buf, strlen(buf));

  /* abAppend(&ab, "\x1b[H", 3); */
  abAppend(&ab, "\x1b[?25h", 6);
  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}

void editorSetStatusMessage(
    const char *fmt,
    ...) { // take a format string and variables; ... makes this a variadic
           // function, meaning it can take any # of args;
  va_list ap;
  va_start(ap, fmt); // fmt is passed so address of all args are known;
  vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt,
            ap); // store string in Estatusmsg;
  va_end(ap);
  E.statusmsg_time = time(NULL); // set estatusmsg_time to current time;
}

/*** input ***/

char *editorPrompt(char *prompt, void (*callback)(char *, int)) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);
  size_t buflen = 0;
  buf[0] = '\0';
  while (1) {
    editorSetStatusMessage(prompt, buf);
    editorRefreshScreen();
    int c = editorReadKey();
    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen != 0)
        buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      editorSetStatusMessage("");
      if (callback)
        callback(buf, c);
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        editorSetStatusMessage("");
        if (callback)
          callback(buf, c);
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }
    if (callback)
      callback(buf, c);
  }
}

void editorMoveCursor(int key) { // move cursor with WASD
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

void editorProcessKeypress() { // wait for keypress, handle it;
  static int quit_times = ediSON_QUIT_TIMES;
  int c = editorReadKey();
  switch (c) {
  case '\r':
    editorInsertNewline();
    break;

  case CTRL_KEY('q'):
    if (E.dirty && quit_times > 0) {
      editorSetStatusMessage("WARNING!!! File has unsaved changes. "
                             "Press Ctrl-Q %d more times to quit.",
                             quit_times);
      quit_times--;
      return;
    }
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
    exit(0);
    break;

  case CTRL_KEY('s'):
    editorSave();
    break;

  case HOME_KEY:
    E.cx = 0;
    break;

  case END_KEY:             // end of the line
    if (E.cy < E.numrows) { // make end key work;
      E.cx = E.row[E.cy].size;
    }
    break;

  case CTRL_KEY('f'):
    editorFind();
    break;

  case BACKSPACE:
  case CTRL_KEY('h'):
  case DEL_KEY:
    if (c == DEL_KEY) {
      editorMoveCursor(ARROW_RIGHT);
    }
    editorDelChar();
    break;

  case PAGE_UP:
  case PAGE_DOWN: {     // end of page;
    if (c == PAGE_UP) { // make pageup and pagedown work;
      E.cy = E.rowoff;
    } else if (c == PAGE_DOWN) {
      E.cy = E.rowoff + E.screenrows - 1;
      if (E.cy > E.numrows)
        E.cy = E.numrows;
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
  case CTRL_KEY('l'):
  case '\x1b':
    break;
  default:
    editorInsertChar(c);
    break;
  }
  quit_times = ediSON_QUIT_TIMES;
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
  E.dirty = 0;
  E.filename = NULL;     // set to null so if there's no file given;
  E.statusmsg[0] = '\0'; // empty so there's no default msg;
  E.statusmsg_time = 0;
  E.syntax = NULL;
  E.linenum_indent = 6;
  if (getWindowSize(&E.raw_screenrows, &E.raw_screencols) == -1) {
    die("getWindowSize");
  }
  E.screenrows =
      E.raw_screenrows - 2; // make two lines of space at the end of the screen;
  E.screencols = E.raw_screencols;
}

int main(int argc, char *argv[]) {
  enableRawMode();
  initEditor();
  if (argc >= 2) {
    if (strstr(argv[1], ".json") != NULL) {
      editorOpenJSON(argv[1]);
    } else {
      editorOpen(argv[1]);
    }
  }
  // help msg with quit keybinds;
  editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");
  while (1) {
    editorRefreshScreen();
    editorProcessKeypress();
  }
  return 0;
}
