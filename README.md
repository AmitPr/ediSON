# ediSON
###### *by Carlos Hernandez, Eisuke Hirota, Amit Prasad, Period 10*

## What is ediSON?
##### A glorified JSON text editor. A J<ins>SON</ins> <ins>edi</ins>tor.
JSON, or JavaScript Object Notation, is a format that has widespread use in various technologies, most notably, the web. Our project is an editor for JSON files written in C so that users can edit, parse, and serialize JSON files with ease.

## Build & Run Instructions:
After cloning and `cd`-ing into the cloned directory:
```bash
make #Generates ./ediSON executable.
./ediSON example.json #Or any other file that you'd like to edit.
```

## How did we make it?
### Topics used:
* Files - Reading, Tokenizing, and Serializing to/from files.
* Memory Allocation - Allocating memory to represent JSON at runtime, as well as storing rows for the editor. Opening memory blocks as `FILE*` objects using `open_memstream`.
* File stats - Displaying useful information such as last modified time, and file size.
* Signals - CLI Editor goes into terminal raw-mode to intercept signals.

### Breakdown of who did what:
Amit
* JSON Parser/Serializer/Formatter  

Eisuke & Carlos
* CLI Editor, Syntax Highlighting, File I/O

### Data Structures used:
* Structs for JSON Nodes and Editor Rows
* Array Lists for JSON Arrays
* Variable Length Strings for representing each line and for representing JSON strings

## Timeline:
* 1/17 Finish Basic JSON Parser/Serializer, Finish setup for Text Editor
* 1/19 Add Raw mode
* 1/20 Add input + output
* 1/21 Add the viewer + editor
* 1/22 Add search
* 1/23 Finish CLI for editing any file
* 1/24 Integrating JSON parser/serializer with text editor. 

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
* 1/23: Continued up to step 156; adding syntax highlighting with digits, able to edit any file (Karl)  
* 1/23: Started trying to patch memory leaks in JSON Parser/Serializer (Amit)  
* 1/24: Patched several memory leaks in JSON Parser/Serializer (Amit)  
* 1/24: Finished text editor, added syntax highlighting with strings, JSON keys, elements of array, line-numbering. (Eisuke, Karl, Amit)  
* 1/24: Integrated text editor and JSON Parser/Serializer, Error highlighting (Eisuke, Karl, Amit)  
