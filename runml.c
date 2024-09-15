//  CITS2002 Project 1 2024
//  Student1:   23630652    Zac Doruk Maslen
//  Student2:   24000895    Alexandra Mennie
//  Platform:   Ubuntu Linux

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// defining array size to take in .ml lines
#define MY_SIZE 1000

// initalise different token types for our lexer, values described in comments
typedef enum { 
    TknIdentifier, 
    TknNumber, 
    TknFloat, 
    TknFunction, // "function"
    TknPrint, // "print"
    TknReturn, // "return"
    TknComment, // "#"
    TknNewline, // "\n" 
    TknTab, // "TABx" where x is the number of tabs so far through perusal
    TknAssignmentOperator, // "<-"
    TknTermOperator, // "+" or "-"
    TknFactorOperator, // "*" or "/"
    TknBracket, // "(" or ")"
    TknComma, // ","
    TknEnd // "END" 
} TknType;

// define structure for "Token" as a 'type' and 'value' pair
typedef struct { 
    TknType type;
    char value[100]; } // size set as 100 as we shouldn't encounter a value more than 100 bytes long  
Token;

// node types for AST 
typedef enum {
    nodeNumber,
    nodeIdentifier,
    nodeOperator,
    nodeFunctionCall,
    nodeFunctionDef,
    nodeReturn,
    nodeAssignment,
    nodePrint
} NodeType;

// further definition of each operator
typedef enum {
    operAdd,
    operSubtract,
    operDivide,
    operMultiply
} OperType;

// AST Structure
typedef struct AstNode {
    NodeType type;
    union {
        double number;
        char *identifier;
        struct {
            OperType oper;
            struct AstNode *left;
            struct AstNode *right;
        } operator;
        struct {
            char *name;
            struct AstNode *arguments;
        } functionCall;
        struct {
            char *name;
            struct AstNode *arguments;
            struct AstNode *body;
        } functionDef;
        struct {
            struct AstNode *value;
        } returnStatement;
        struct {
            char *identifier;
            struct AstNode *value;
        } assignment;
        struct {
            struct AstNode *value;
        } print;
    } data;
} AstNode;

// ###################################### TOKENISATION START ######################################

// initalise "Tokens" array
Token Tokens[1000]; // array that stores all tokens generated by the lexer
int TknIndex = 0; // integer value that keeps track of our current position in array

// function to add tokens to our "Tokens" array
void addToken(TknType type, const char *value) { 
    Tokens[TknIndex].type = type; // sets type
    strcpy(Tokens[TknIndex].value, value); // sets value 
    TknIndex++; // increases token index/position pointer 
}

// FOR TESTING PURPOSES - print the token
void print_token(Token token) {
    printf("Token Type: %d, Value: %s\n", token.type, token.value);
}

// function to check validity of identifiers
bool isValidIdentifier(const char *str) {
    int length = strlen(str);
    if (length < 1 || length > 12) { // check length between 1 and 12
        return false;
    }
    for (int i = 0; i < length; i++) { // check all characters are lowercase and alphabetical 
        if (!islower(str[i]) || !isalpha(str[i])) {
            return false;
        }
    }
    return true;
}

int lineCount(FILE *file) {
    int count = 0;
    char c;
    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            count++;
        }
    }
    rewind(file);
    return count;
}

// lexer function to convert code to token list
void tokenize(const char *code) {
    const char *pointer = code; // accesses character in code
    int IndentLevel = 0; // integer to track indent level

    while (*pointer) {

        // check for comments first (to remove them from consideration and avoid errors later on)
        if (*pointer == '#') {
            addToken(TknComment, "#");  // 

            // skip everything until the newline character
            while (*pointer && *pointer != '\n') {
                pointer++;
            }
            continue;  // skip until newline, negating whole comment line from tokens array
            }

        // check for blank spaces such as tab and newline
        if (isspace(*pointer)) { 
            if (*pointer == '\t') { // if tab exists
                char TempBuffer[100]; // creates buffer to store characters, size should not exceed 100
                sprintf(TempBuffer, "TAB%d", IndentLevel++); // string creation of value for tab token
                addToken(TknTab, TempBuffer); // adds buffer as token
            } 
            else if (*pointer == '\n') { //if newline exists
                addToken(TknNewline, "\n"); 
                IndentLevel = 0;
            }
            pointer++; // loops if ' ' appears
            continue;
            }

        // check for numbers (real constant values)
        else if (isdigit(*pointer)) { // if integer number or float (aka. realconstant) exists
            char TempBuffer[100] = {0}; // intialises all characters in buffer to 0 
            int i = 0; // iterator 
            bool hasDecimalPoint = false; // needed for syntax error checking
            
            while (isdigit(*pointer)) { // increment and extract digits before decimal point
                 TempBuffer[i++] = *pointer++; // 
            }

            if (*pointer == '.') { // handle floats
                if (hasDecimalPoint) { // check for if multiple decimal points exist
                    fprintf(stderr, "! Syntax Error: Multiple decimal points in number.\n Recommendation: Check all numbers for incorrect format.\n");
                    exit(1);
                }

                hasDecimalPoint = true;
                TempBuffer[i++] = *pointer++; // decimal point added 

                while (isdigit(*pointer)) { // increment and extract digits after decimal point
                    TempBuffer[i++] = *pointer++;
                }
                
                addToken(TknFloat, TempBuffer); // if float exists

            } else { // if number exists
                addToken(TknNumber, TempBuffer);
            }

            /* check for invalid characters after the number
            if (!isdigit(*pointer)) {
                fprintf(stderr, "! Syntax Error: Invalid character '%c' after number.\n Recommendation: Check all numbers for invalid characters. Ensure all operators, identifiers, constants and words are properly seperated by spaces. \n", *pointer);
                exit(1);
            }
            */

            // I think this may be the better way of checking for invalid characters after a number,
            // bc in ml its acceptable to find whitespaces after tokens -- but just double check LMAO

            if (!isspace(*pointer) && *pointer != '+' && *pointer != '-' && *pointer != '*' && *pointer != '/' && 
            *pointer != '(' && *pointer != ')' && *pointer != ',' && *pointer != '\0') {
                fprintf(stderr, "! Syntax Error: Invalid character '%c' after number.\nRecommendation: Ensure that numbers are followed by operators, spaces, or valid symbols.\n", *pointer);
                exit(1);
            }

        }

        // check for strings (identifiers or reserved words)
        else if (isalpha(*pointer) && islower(*pointer)) { // alphabetical lower case only
            char TempBuffer[100] = {0};
            int i = 0;
            while (isalnum(*pointer)|| *pointer == '_') { // more general isalnum() allows for us to pass invalid strings into identifier checker, meaning that this specific error can be accurately flagged
            TempBuffer[i++] = *pointer++; } 
            TempBuffer[i] = '\0'; // Null-terminate the buffer

            if (strcmp(TempBuffer, "function") == 0) { // if function keyword exists
                addToken(TknFunction, TempBuffer);
            }
            else if (strcmp(TempBuffer, "print") == 0) { // if print keyword exists
                addToken(TknPrint, TempBuffer);
            } 
            else if (strcmp(TempBuffer, "return") == 0) { // if return keyword exists
                addToken(TknReturn, TempBuffer);
            }
            else if (isValidIdentifier(TempBuffer)) { // if valid identifier exists 
                addToken(TknIdentifier, TempBuffer);
            } 
            else { // if invalid string exists
                fprintf(stderr, "! Syntax Error: Invalid characters in identifier or string.\n Recommendation: Ensure all characters are lower case. Identifiers should be alphabetical only and between 1 and 12 characters long. \n");
                exit(1);
            }
            continue;
            }             

        // check for mathematical operators
        else if (*pointer == '+' || *pointer == '-' || *pointer == '*' || *pointer == '/' || *pointer == '(' || *pointer == ')'|| *pointer == ',') { // if valid character
            char TempBuffer[2] = {*pointer, '\0'};
            TknType TknType;
                switch (*pointer) {
                    case '+':
                    case '-':
                        TknType = TknTermOperator;  // if '+' and '-' (term operators) exists
                        break;
                    case '*':
                    case '/':
                        TknType = TknFactorOperator;  // if '*' and '/' (factor operators) exists
                        break;
                    case '(':
                    case ')':
                        TknType = TknBracket;  // if '(' and ')' (brackets) exists
                        break;
                    case ',':
                        TknType = TknComma;  // if ',' (comma) exists
                        break;
        } 
        
        // was missing addToken after each case, so added here
        addToken(TknType, TempBuffer);
        pointer++;

        // check for assignment operator
        } else if (*pointer == '<' && *(pointer + 1) == '-') { // if "<-" operator exists
            addToken(TknAssignmentOperator, "<-");
            pointer += 2;
        }

        // check for all other characters
        else { 
            fprintf(stderr, "! Syntax Error: Illegal character '%c' exists in file.\n Recommendation: remove invalid symbols and all uppercase to fix. \n", *pointer); // just added what character its throwing an error for 
            exit(1);
        }
    }
}

// function to read contents of a .ml file
int readFile(char *filename) {

    // opening file for reading
    FILE *file = fopen(filename, "r");
    
    // error checking: file does not exist
    if (file == NULL) {
        fprintf(stderr, "! Error: Could not open file %s\n", filename);
        return -1;
    }

    // buffer holds each line
    char line[MY_SIZE]; 
    
    // file reading logic
    while (fgets(line, sizeof(line), file) != NULL) {
        // remove new line char 
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // tokenize
        // printf("Line: %s\n", line); // debug
        tokenize(line);
    }

    // error checking: file is empty
    if (ferror(file)) {
        printf("! Error: Could not read file %s\n", filename);
        fclose(file);
        return -1;
    }
    
    // closes file
    fclose(file);
    
    addToken(TknEnd, "END");

    return 0;
}

//need to add a checker that checks there are max 50 unique identifiers

// ###################################### TOKENISATION END ######################################

// ###################################### PARSING START ######################################

/* 
a parser organises tokens into a structured form based on the grammer of the language
i.e. goes through the following steps: token organisation, 
validation (if sequence of tokens forms valid constructs according to ml specs),
and then generates syntax tree
i.e. checks if an assignment is correct (i.e. x = 5 is valid but 5 = x is not)
and then generates a syntax tree based on the valid constructs 
*/


// Array to store function names
char ExistingFunctions[50][256]; // based on max 50 unique identifiers
int FunctionsCount = 0;  // Counter for the number of functions

// Function to add function names to the array
void addFunctionName(const char* identifier) {
    if (FunctionsCount < 50) { //
        strcpy(ExistingFunctions[FunctionsCount++], identifier);
    }

//parsing overall
void pProgramItem() {
if (currentTkn().type == TknFunction) {
        paFuncDef(); // go to function definitions
else{
    pStmt() // go to statements
}

}

// parsing for existance of a function definition
void pFuncDef() {
        pNextToken();  // Go to next token
        if (pCurrentTkn().type == TknIdentifier) {
            addFunctionName(pCurrentTkn().value); // Store function name
            pNextToken();
// Expecting open bracket
            if (pCurrentTkn().type == TknLBracket) {
                pNextToken();  
                if (pCurrentTkn).type == TknRBracket) { // expect closed bracket
                    pNextToken();  // void function exists here
                    pFuncStmt(); // parse for function statements
                } 
                else if {pCurrentTkn().type == TknIdentifier) {
                    //do function thingys
                }
                    printf("Syntax Error: Expected ')' after function parameters\n");
                    exit(1);
                }
            } else {
                printf("Syntax Error: Expected '(' after function name\n");
                exit(1);
            }
        } else {
            printf("! SYNTAX ERROR: Expected identifier for function name\n");
            exit(1);
        }
    }
}

pFuncStmt(){
    //expect newline for end of void function
    if (pCurrentTkn).type == TknNewline) {
        pNextToken();
        if (pCurrentTkn).type == TknTab {
            pNextToken();
            pStmt();
        }
        else {
        printf("! SYNTAX ERROR: Expected tab/indentation before function statements. \n");
        exit(1);
        }
    }
    else {
    printf("! SYNTAX ERROR: Expected new line for function statements. \n");
    exit(1);
    }
}

// parsing statements - print, return, assignment, functioncall
void pStmt() {
if (currentTkn().type == TknPrint) {
        pPrint();
    } else if (currentTkn().type == TknReturn) {
        pReturn();
    } else if (currentTkn().type == TknIdentifier) {
        pAssignment();
    } else {
        printf("! Syntax Error: Unexpected statement starting term. \n");
        exit(1);
    }
}


int pCurrentTknIndex = 0;

// Move to the next token
void pNextToken() {
    pCurrentTknIndex++;
}

// Fetch the current token
Token pCurrentTkn() {
    return tokens[pCurrentTknIndex];
}


// need to work on below
// calls the parser that corresponds to the token type
void Parser() {
    if (pCurrentTknIndex >= 1000) {
        fprintf(stderr, "! Error: Token index out of bounds\n");
        exit(1);
    }

    // current token
    Token token = Tokens[pCurrentTknIndex];

}

void parser() {
    while(TknIndex < 1000 && Tokens[TknIndex].type != TknEnd) {
        pToken();
    };
}

// ###################################### PARSING END ######################################

// ###################################### TRANSLATION TO C START ######################################

void writeCFile() {
    FILE *cFile = fopen("mlProgram.c", "w");
    if (cFile == NULL) {
        fprintf(stderr, "! Error: Could not create C file\n");
        return;
    }

    // writing to C file
    fprintf(cFile, "#include <stdio.h>\n\n");
    fprintf(cFile, "int main() {\n");
    fprintf(cFile, "toC(cFile, root);");
    fprintf(cFile, "    return 0;\n");
    fprintf(cFile, "}\n");

    fclose(cFile);
}

// defining translation to C function
void toC(FILE *cFile) {

}

void compileAndRunInC() {
    system("gcc -o mlProgram mlProgram.c");
    system("./mlProgram");
}

// function to remove created C file and exec file
void cleanupAfterExec() {
    remove("mlProgram.c");
    remove("mlProgram");
}

// ###################################### TRANSLATION TO C END ######################################

int main(int argc, char *argv[]) {
    // error checking, if no. of args is less than 2 
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename.ml>\n", argv[0]); // changed to fprintf to print to stderr instead of default data stream
        return 1;
    }

    // the name of the file is at initial argument provided
    char *filename = argv[1];

    // checks if file name is .ml
    size_t length = strlen(filename); // unsigned datatype, good for storing str length 
    if (length < 3 || strcmp(filename + length - 3, ".ml") != 0) {
        fprintf(stderr, "! Error: File name must end with '.ml'\n");
        return 1;
    }

    // read the file
    int status = readFile(filename); // also doing tokenisation

    if (status == -1) {
        return 1;
    }

    // debugging tokens
    printf("Tokens after tokenisation\n");
    for (int i = 0; i < TknIndex; i++) {
        print_token(Tokens[i]);
    }

    // parsing
    parser();

    // translation to C
    writeCFile();
    compileAndRunInC();
    cleanupAfterExec();

    return 0;
}
