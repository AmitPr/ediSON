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

Eisuke
* CLI Editor  

Carlos
* Working with files  

### Data Structures used:
* Structs for JSON Nodes
* Array Lists for JSON Arrays
* Variable Length Strings for representing each line and for representing JSON strings

## Timeline:
* 1/14 Finish Basic Json Parser/Serializer
* 1/17 Finish CLI for editing any file
* 1/19 Finish adding features specific to json files
## Prerequisites:
* None (At the moment)

## Instructions:
*N/A*

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
