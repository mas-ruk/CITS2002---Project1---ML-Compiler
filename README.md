# CITS2002 Project 1: ML Compiler

## Goal of the Project

The objective of this project is to implement a C11 program named `runml` that performs the following tasks:

1. Accepts a command-line argument specifying the pathname of a text file containing a program written in the ML language.
2. Accepts optional command-line arguments to be passed to the transpiled program when it is executed.
3. Checks the syntax of the ML program.
4. Translates the valid ML program into a C11 program.
5. Compiles the resultant C program.
6. Executes the compiled program.

## The ML Language

ML programs have the following characteristics:

- Programs are written in text files with names ending in `.ml`.
- Statements are written one per line, with no terminating semicolon.
- The `#` character introduces comments that extend until the end of the line.
- Only real numbers are supported (e.g., `2.71828`).
- Identifiers (variable and function names) consist of 1 to 12 lowercase alphabetic characters (e.g., `budgie`).
- At most 50 unique identifiers are allowed in any program.
- Variables are automatically initialized to `0.0` and do not need to be defined before use.
- Special variables like `arg0`, `arg1`, etc., provide access to command-line arguments (real-valued numbers).
- Functions must be defined before being called in an expression.
- Statements in a function's body must be indented with a tab character.
- Functions may have zero or more formal parameters.
- Identifiers in a function are local to that function and become unavailable after its execution.
- Programs execute statements from top to bottom, with function calls as the only form of control flow.

## Steps to Compile and Execute an ML Program

1. Edit a text file named, for example, `program.ml`.
2. Pass `program.ml` as a command-line argument to your `runml` program.
3. `runml` validates the ML program, reporting any errors.
4. `runml` generates C11 code in a file named, for example, `ml-12345.c` (where `12345` could be a process ID).
5. `runml` uses your system's C11 compiler to compile `ml-12345.c`.
6. `runml` executes the compiled C11 program `ml-12345`, passing any optional command-line arguments (real numbers).
7. `runml` removes any files that it created.

## Project Requirements

- Your project must be written in C11, in a single source code file named `runml.c`.
- It must behave as a standard utility program:
  - Check its command-line arguments.
  - Display a usage message on error.
  - Print 'normal' output to `stdout` and error messages to `stderr`.
  - Terminate with an exit status reflecting its execution success.
- The project must not rely on any third-party libraries, only system-provided libraries (OS, C11, and POSIX functions).
- All syntax errors in invalid ML programs must be reported via `stderr`, beginning with the `!` character. 
  - Note: Syntax errors in expressions do not need to be validated.
- The only 'true' output produced by the translated and compiled program should be from executing ML's `print` statement.
  - Debug printing should appear on lines beginning with the `@` character.
- Numbers should be printed as exact integers without decimal places or with exactly 6 decimal places.
- Dynamic memory allocation (e.g., `malloc()`) is optional and will not earn extra marks.
