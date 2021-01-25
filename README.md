# ediSON
###### *by Carlos Hernandez, Eisuke Hirota, Amit Prasad, Period 10*

## What is ediSON?
##### A glorified JSON text editor. A J<ins>SON</ins> <ins>edi</ins>tor.
JSON, or JavaScript Object Notation, is a format that has widespread use in various technologies, most notably, the web. Our project is an editor for JSON files written in C so that users can edit, parse, and serialize JSON files with ease.

## How did we make it?
### Topics used:
* Files - Reading, Parsing, and Serializing to/from files.
* Memory Allocation - Allocating memory to represent JSON at runtime
* File stats - Displaying useful information such as last modified time, and file size.
* Signals - CLI Editor should ask to save before exiting, and be able to intercept other signals gracefully.

### Breakdown of who did what:
Amit
* JSON Parser/Serializer  

Eisuke & Carlos
* CLI Editor

### Data Structures used:
* Structs for JSON Nodes
* Array Lists for JSON Arrays
* Variable Length Strings for representing each line and for representing JSON strings

## Timeline:
* 1/17 Finish Basic Json Parser/Serializer, Finish setup for Text Editor
* 1/19 Add Raw mode
* 1/20 Add input + output
* 1/21 Add the viewer + editor
* 1/22 Add search
* 1/23 Finish CLI for editing any file
* 1/24 Finish project

## Instructions:
```bash
$ git clone https://github.com/AmitPr/ediSON.git
$ make
$ # optional argument of ARGS=<FILENAME>
$ make run ARGS=example.json
$ make clean
```

## Files & Function Signatures
### ediSON.c
- It is 1200 lines long, so there is a lot to cover.

defines
```c
// line 29: enum editorKey -> keys to move cursor
// line 42: enum editorHighlight -> matches to color
// The following are flags to determine if filetype should be highighted
#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

```

data
```c
struct editorSyntax {...}; // line 56: keys information about file

typedef struct erow {...} erow; // line 63: stores information about current line

struct editorConfig {...}; //  line 71: Keeps info about editor

```


filetypes
```c

char *json_extensions[] = {...}; // line 95: Lists json file extensions

char *JSON_keywords[] = {...}; // line 97: keywords for json

struct editorSyntax HLDB[] = {...}; // line 99: Sets up highlighting
```

prototypes
```c
void editorSetStatusMessage(const char *fmt, ...); // line 996: Sets msg to statusbar
void editorRefreshScreen(); //line 969: Updates screen after a char is typed
char *editorPrompt(char *prompt, void (*callback)(char *, int)); // line 1010: brings up MSG prompt
```

terminal
```c
void die(const char *s); // line 112: Sets an errors

void disableRawMode(); // line 120: sets canonical mode

void enableRawMode(); // line 127: sets terminal to raw mode

int editorReadKey(); // line 151: reads a key and returns appropriate value

int getCursorPosition(int *rows, int *cols); // line 216: sets address of rows and cols to current cursor position

int getWindowSize(int *rows, int *cols); // line 246: gets maximum size of cols and rows

```

Syntax highlighting
```c
int is_separator(int c); // line 263: checks if char is separator

void editorUpdateSyntax(erow *row); // line 267: adds highlighting to row

int editorSyntaxToColor(int hl, int checkBold); // line 362: matches syntax to color in struct editorSyntax

void editorSelectSyntaxHighlight(); // line 385: If json then it adds highlighting 
```

Row operations
```c
int editorRowCxToRx(erow *row, int cx); // line 412: converts typed chars to render chars. e.g. to show actual tab size.

int editorRowRxToCx(erow *row, int rx); // line 424: converts render chars to typed chars

void editorUpdateRow(erow *row); // line 440: Copies characters from char to render

void editorInsertRow(int at, char *s, size_t len); // line 464: inserts row at index

void editorFreeRow(erow *row); // line 485: frees memory

void editorDelRow(int at); // line 491: deletes row

void editorRowInsertChar(erow *row, int at, int c); // line 501: appends char to row

void editorInsertNewline(); // line 513: handles <Return> key

void editorRowDelChar(erow *row, int at); // line 528:  deletes specific char at index

void editorRowAppendString(erow *row, char *s, size_t len); // line 538:  Appends string to end of row
```


Editor operations
```c
void editorInsertChar(int c); // line 548: inserts char at cursor

void editorDelChar(); // line 556: deletes char left of cursor
```


File I/O
```c
char *editorRowsToString(int *buflen); // line 577: converts array of erow into single string

void editorOpenJSON(char *filename);  // line 595: Tokens and adds syntax of json file

void editorOpen(char *filename); // line 643: Opens file and gets line by line

void editorSave(); // line 668: Allows file to be saved
```


Find
```c
void editorFindCallback(char *query, int key); // line 699: Functions allows for incremental search

void editorFind(); // line 753: stores query and frees it
```

Append buffer
```c
struct abuf {...}; // line 771: Dynamic strings

void abAppend(struct abuf *ab, const char *s, int len); // line 779 appends to abuf struct

void abFree(struct abuf *ab);  // line 791 free dynamic string;

```

Output
```c
void editorScroll(); // line 794 Allows window to be scrolled

void editorDrawRows(struct abuf *ab); // line 814 draws line numbers and tildes to editor

void editorDrawStatusBar(struct abuf *ab); // line 910 draws status bar

void editorDrawMessageBar(struct abuf *ab); // line 942 draw msg bar

void editorUpdateLinenumIndent(); // line 951 updates line number
```

Input
```c
/*** input ***/
void editorMoveCursor(int key); // line 1048: moves cursor with arrow keys

void editorProcessKeypress(); // lines 1085: handles key presses
```

Init
```c
void initEditor(); // line 1164: Initializes fields of struct E

int main(int argc, char *argv[]); // line 1186 runs the code
```

## DEVLOG
* 1/14: Finished reading in files to string, started working on JSON Parser (Amit)
* 1/17: Finish working on basic JSON Parser - Just objects, not arrays. (Amit)
* 1/17: Start working on CLI text editor (Eisuke & Karl)
* 1/18: Finish working on JSON array parser, as well as nested objects and arrays (Amit)
* 1/19: Adding comments to texteditor, continue fleshing it out (Eisuke & Karl)
* 1/19: Realize that the JSON Parser is fundamentally flawed after 4 hours of bug fixing (Amit)
* 1/20: Continued working on texteditor (Karl)
* 1/20: Start rewriting JSON Parser to use a tokenizer-like model instead (Uses less memory, simpler, more powerful) (Amit)
* 1/21: Finish writing JSON Tokenizer for everything but numbers, bug fixes on tokenizer (Amit)
* 1/22: Continued up to step 100; text editor is able to take in a file, cursor can move through the file, added a status bar and a msgbar (Eisuke & Karl)
* 1/23: Continued up to step 156; adding syntax highlighting with digits, able to edit any file (Karl); 
* 1/24: Finished text editor, added syntax highlighting with strings, keys specifically, elements of array, line-numbering, integrated JSON tokenizer with texteditor (Eisuke, Karl, Amit)
