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
#define MAX_NODES 1000

// initalise different token types for our lexer, values described in comments
typedef enum { 
    TknIdentifier, 
    TknNumber, 
    TknFloat, 
    TknFunction, // "function"
    TknPrint, // "print"
    TknReturn, // "return"
    TknNewline, // "\n" 
    TknTab, // "TABx" where x is the number of tabs so far through perusal
    TknAssignmentOperator, // "<-"
    TknTermOperator, // "+" or "-"
    TknFactorOperator, // "*" or "/"
    TknLBracket, // "(" 
    TknRBracket, // ")"
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
    nodeProgram, // actual root program
    nodeStmt, 
    nodeExpression,
    nodeTerm,
    nodeFactor,
    nodeFunctionCall,
    nodeFunctionDef,
    nodeReturn,
    nodeAssignment,
    nodePrint
} NodeType;

// AST Structure
typedef struct AstNode {
    NodeType type;
    union {
        // to account for lines/statements in program we need to create an overarching program node
        struct {
            struct AstNode **programItems; // holds all statements in program
            int lineCount;
        } program;

                // func def node
        struct {
            char *identifier;
            char** params; // array of parameter names
            int paramCount;
            struct AstNode **stmt; // array of statment nodes
            int stmtCount;
        } funcDef;
        
        // statement node
        struct {
        NodeType type; // Add a type to differentiate between statement types
            union {
                struct {
                    struct AstNode *exp; // For assignment statements
                    char *identifier; // Variable name
                } assignment;

                struct {
                    struct AstNode *exp; // For print statements
                } print;

                struct {
                    struct AstNode *exp; // For return statements
                } returnStmt;

                struct {
                    struct AstNode *funcCall; // For function calls
                } funcCall;
            } data;
        } stmt;
        
        // expression node
        struct {
            struct AstNode *lVar; // 
            char *oper; // + or -
            struct AstNode *rVar;
        } Expression;
        
        // term node
        struct {
            struct AstNode *lVar;
            char *oper; // x or /
            struct AstNode *rVar;
        } term;
        
        // factor node
        struct {
            float constant; // using float as we only require 6 digits of precision
            char* identifier;
            struct AstNode *funcCall;
            struct AstNode *exp; // expressions in parentheses
        } factor;

        // func call node
        struct {
            char* identifier;
            struct AstNode **args;
            int argCount;
        } funcCall;
        
        // assignment node
        struct {
            char* identifier; // var
            struct AstNode *exp;
        } assignment;
        
        // print
        struct {
            struct AstNode *exp;
        } print;
        
        // return
        struct {
            struct AstNode *exp;
        } returnStmt;
    } data;
} AstNode;

// we didnt have a way of representing the statements each in the program so here's a node for it
struct {
    struct AstNode **statements; // array for statements
    int statementCount;
} statementSeq;

// ###################################### TOKENISATION START ######################################

// FOR TESTING PURPOSES - print the token
void print_token(Token token) {
    printf("Token Type: %d, Value: %s\n", token.type, token.value);
}


// initalise "Tokens" array
Token Tokens[1000]; // array that stores all tokens generated by the lexer
int TknIndex = 0; // integer value that keeps track of our current position in array
int TknCount = 0; // stores the number of tokens in our array

// function to add tokens to our "Tokens" array
void addToken(TknType type, const char *value) { 
    Tokens[TknIndex].type = type; // sets type
    strcpy(Tokens[TknIndex].value, value); // sets value 
    TknIndex++; // increases token index/position pointer
    TknCount++; // increment token count
    print_token(Tokens[TknIndex - 1]);
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
    const char *pointer = code;
    int IndentLevel = 0;

    while (*pointer) {
        if (*pointer == '#') {
            while (*pointer && *pointer != '\n') {
                pointer++;
            }
            continue;
        }

        if (isspace(*pointer)) {
            if (*pointer == '\t') {
                char TempBuffer[100];
                sprintf(TempBuffer, "TAB%d", IndentLevel++);
                addToken(TknTab, TempBuffer);
            } else if (*pointer == '\n') {
                addToken(TknNewline, "\n");
                IndentLevel = 0;
            }
            pointer++;
            continue;
        }

        // Number handling
        if (isdigit(*pointer)) {
            char TempBuffer[100] = {0};
            int i = 0;
            bool hasDecimalPoint = false;

            while (isdigit(*pointer)) {
                TempBuffer[i++] = *pointer++;
            }

            if (*pointer == '.') {
                hasDecimalPoint = true;
                TempBuffer[i++] = *pointer++;
                while (isdigit(*pointer)) {
                    TempBuffer[i++] = *pointer++;
                }
            }

            TempBuffer[i] = '\0'; // Ensure null-termination

            if (hasDecimalPoint) {
                addToken(TknFloat, TempBuffer);
            } else {
                addToken(TknNumber, TempBuffer);
            }

            // Check next character after number
            if (!isspace(*pointer) && !strchr("+-*/()&,", *pointer) && *pointer != '\0') {
                fprintf(stderr, "! Syntax Error: Invalid character '%c' after number.\n", *pointer);
                return; // or exit
            }
            continue;
        }

        if (isalpha(*pointer) && islower(*pointer)) {
        char TempBuffer[100] = {0};
        int i = 0;

        // Extract the identifier or reserved word
        while (isalnum(*pointer) || *pointer == '_') {
            if (i < sizeof(TempBuffer) - 1) {
                TempBuffer[i++] = *pointer++;
            } else {
                fprintf(stderr, "! Syntax Error: Identifier too long.\n");
                exit(1);
            }
        }
        TempBuffer[i] = '\0'; // Null-terminate the string

        // Check against reserved words first
        if (strcmp(TempBuffer, "function") == 0) {
            addToken(TknFunction, TempBuffer);
        } else if (strcmp(TempBuffer, "print") == 0) {
            addToken(TknPrint, TempBuffer);
        } else if (strcmp(TempBuffer, "return") == 0) {
            addToken(TknReturn, TempBuffer);
        } else if (isValidIdentifier(TempBuffer)) {
            addToken(TknIdentifier, TempBuffer);
        } else {
            fprintf(stderr, "! Syntax Error: Invalid identifier.\n");
            exit(1); // Terminate on error
        }
        return; // Ensures we exit this check to prevent further processing
    }

        // Mathematical operators
        if (strchr("+-*/(),", *pointer)) {
            char TempBuffer[2] = {*pointer, '\0'};
            TknType type;
            switch (*pointer) {
                case '+':
                case '-':
                    type = TknTermOperator;
                    break;
                case '*':
                case '/':
                    type = TknFactorOperator;
                    break;
                case '(':
                    type = TknLBracket;
                    break;
                case ')':
                    type = TknRBracket;
                    break;
                case ',':
                    type = TknComma;
                    break;
                default:
                    fprintf(stderr, "! Syntax Error: Illegal character '%c'.\n", *pointer);
                    return; // or exit
            }
            addToken(type, TempBuffer);
            pointer++;
            continue;
        }

        // Assignment operator
        if (*pointer == '<' && *(pointer + 1) == '-') {
            addToken(TknAssignmentOperator, "<-");
            pointer += 2;
            continue;
        }

        // All other characters
        fprintf(stderr, "! Syntax Error: Illegal character '%c'.\n", *pointer);
        return; // or exit
    }
}


// function to read contents of a .ml file
int readFile(const char *filename) {

    // opening file for reading
    FILE *file = fopen(filename, "r");
    
    // error checking: file does not exist
    if (file == NULL) {
        fprintf(stderr, "@ Error: Could not open file %s\n", filename);
        return -1;
    }

    // buffer holds each line
    char line[MY_SIZE]; 
    
    // file reading logic
    while (fgets(line, sizeof(line), file) != NULL) {
        // tokenize
        // printf("Line: %s\n", line); // debug
        tokenize(line);
    }

    // error checking: file is empty
    if (ferror(file)) {
        printf("@ Error: Could not read file %s\n", filename);
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

// Compiler throwing hissy fit hence this
char *strdup(const char *s) {
    size_t len = strlen(s);
    char *dup = malloc(len + 1);
    if (dup) {
        strcpy(dup, s);
    }
    return dup;
}

// Prototype for the parser functions
AstNode* pFuncCall();
AstNode* pExpression();
AstNode* pProgram();

AstNode nodes[MAX_NODES]; // not using malloc, static allocation
int pCurrentTknIndex = 0;
int nodeCount = 0;

// Fetch the current token
Token pCurrentTkn() {
    return Tokens[pCurrentTknIndex];
}

// here is a function to grab the next token
Token getNextTkn() {
    return Tokens[pCurrentTknIndex++];
}

// Increment token index
void pMoveToNextTkn() {
    pCurrentTknIndex++;
}

// add new node from token to tree
AstNode* createNode(NodeType type){
    if (nodeCount < MAX_NODES) {
        AstNode *node = &nodes[nodeCount++];
        node -> type = type;
        return node;
    } else {
        fprintf(stderr, "@ Error: Maximum no. of nodes reached. Memory allocation exhausted.");
        exit(EXIT_FAILURE);
    }
}

// Array to store function names
char ExistingFunctions[50][256]; // based on max 50 unique identifiers
int FunctionsCount = 0;  // Counter for the number of functions

// Function to add function names to the array
void addFunctionName(const char* identifier) {
    if (FunctionsCount < 50) { //
        strcpy(ExistingFunctions[FunctionsCount++], identifier);
    }
}

//Function to check if function identifer within the array
bool doesFunctionExist(const char* funcID) {
    for (int i = 0; i < FunctionsCount; i++) {
        if (strcmp(ExistingFunctions[i], funcID) == 0) {
            return true; // name already exists
        }
    }
return false;
}

AstNode* pFactor() {
    AstNode* factorNode = NULL;
    
    // check for number existence
    if (pCurrentTkn().type == TknNumber) {
        factorNode = createNode(nodeFactor);
        factorNode -> data.factor.constant = atof(pCurrentTkn().value);
        pMoveToNextTkn();
    }
    // check for float existence
    else if (pCurrentTkn().type == TknFloat) {
        factorNode = createNode(nodeFactor);
        factorNode -> data.factor.constant = atof(pCurrentTkn().value);
        pMoveToNextTkn();
    } 
    else if (pCurrentTkn().type == TknIdentifier) {
        // if function call
        if (doesFunctionExist(pCurrentTkn().value)) { 
            factorNode = createNode(nodeFactor);
            factorNode -> data.factor.funcCall = pFuncCall();
        }
        //not function call
        else    {
            factorNode = createNode(nodeFactor);
            factorNode -> data.factor.identifier = strdup(pCurrentTkn().value);
            pMoveToNextTkn();
                if (pCurrentTkn().type != TknNewline && pCurrentTkn().type != TknEnd) {
                printf ("! SYNTAX ERROR: Expected new line after non-function name identifier\n.");
                exit(1);
                }
            }
    } else if (pCurrentTkn().type == TknLBracket) {
        pMoveToNextTkn();
        factorNode = createNode(nodeFactor);
        // check stuff within the brackets 
        factorNode -> data.factor.exp = pExpression();
            if(pCurrentTkn().type != TknRBracket) {
                printf("! SYNTAX ERROR: Invalid factor. Expected ')' after expression.\n");
                exit(1);
            }    
        pMoveToNextTkn(); // consume ')'
    } else {
        printf("! SYNTAX ERROR: Invalid factor. Expected functioncall, real constant, identifer or '(' expression ')'.\n.");
        exit(1);
    }
    return factorNode;
}

// creating that left right operator child tree
AstNode* pTerm(){    
    AstNode* fctrNode = pFactor(); // parse first factor
    // if is factor operator must parse recursively
    if (pCurrentTkn().type == TknFactorOperator) {
        char* oper = strdup(pCurrentTkn().value); // store oper
        pMoveToNextTkn(); // move to next token
        AstNode* rVarNode = pTerm(); // parse next token
        AstNode* termNode = createNode(nodeTerm);
        termNode -> data.term.lVar = fctrNode; // node given lVar property i.e. the left factor 
        termNode -> data.term.rVar = rVarNode; // right variable 
        termNode -> data.term.oper = oper; // operator assigned as well
        return termNode; 
    } return fctrNode;
}

AstNode* pExpression(){
    AstNode* termNode = pTerm(); // parse first term
    if (pCurrentTkn().type == TknTermOperator) {
        char* oper = strdup(pCurrentTkn().value);
        pMoveToNextTkn();
        AstNode* rVarNode = pExpression();
        AstNode* exprNode = createNode(nodeExpression);
        exprNode -> data.Expression.lVar = termNode; 
        exprNode -> data.Expression.oper = oper;
        exprNode->data.Expression.rVar = rVarNode;
        return exprNode;
    } return termNode;
}

#define MAX_ARGS 1000
AstNode* pFuncCall() {
    AstNode* funcCallNode = createNode(nodeFunctionCall);

    // Consume (EDIT: STORE) the function name
    funcCallNode -> data.funcCall.identifier = strdup(Tokens[pCurrentTknIndex - 1].value);

    // throwing errors so lets do some malloc bullcrap
    funcCallNode -> data.funcCall.args = malloc(sizeof(AstNode*) * MAX_ARGS);
    funcCallNode -> data.funcCall.argCount = 0;

    // Check for the left bracket
    if (pCurrentTkn().type == TknLBracket) {
        pMoveToNextTkn();  // Consume '('
        
        // parse parameters
        if (pCurrentTkn().type != TknRBracket) {
            // Parse the first expression or parameter
            AstNode* paramNode = pExpression();
            // ok this is a hell of a line but basically what this is doing
            // is it accesses the funcCallNode that we've created 
            // and then puts in the paramNode at the index identified by argCount as far as i can tell lmao
            // cross fingers it works LMAO
            funcCallNode -> data.funcCall.args[funcCallNode -> data.funcCall.argCount++] = paramNode;

            // Check for additional parameters separated by commas
            while (pCurrentTkn().type == TknComma) {
                pMoveToNextTkn();  // Consume ','
                AstNode* paramNode = pExpression(); // Parse the next parameter
                funcCallNode -> data.funcCall.args[funcCallNode -> data.funcCall.argCount++] = paramNode;
            }
        }
        
        // Check for the right parenthesis ')'
        if (pCurrentTkn().type != TknRBracket) {
            printf("! SYNTAX ERROR: Expected ')' after function parameters.\n");
            exit(1);
        }
        pMoveToNextTkn();  // Consume ')'
    } 
    else {
        printf("! SYNTAX ERROR: Expected '(' after functioncall.\n");
        exit(1);
    }
    return funcCallNode;
}

// parsing over statements
AstNode* pStmt() {
    AstNode* stmtNode = createNode(nodeStmt);
    switch (pCurrentTkn().type){
        case TknIdentifier:
            if (doesFunctionExist(pCurrentTkn().value)) {
                // function call
                stmtNode->data.stmt.data.funcCall.funcCall = pFuncCall();
                stmtNode->type = nodeFunctionCall;
            } else {
                // assignment
                stmtNode -> data.stmt.data.assignment.identifier = strdup(pCurrentTkn().value); // store identifier
                pMoveToNextTkn(); // move to next token
            }
                //check assignment operator correctly exists here
                if (pCurrentTkn().type == TknAssignmentOperator){
                    pMoveToNextTkn(); // consume the assignment operator
                    stmtNode -> data.stmt.data.assignment.exp = pExpression();
                    stmtNode -> type = nodeAssignment;
                
                // validate expression exists for assignment operator 
                    if (!stmtNode->data.assignment.exp) {
                        printf("! SYNTAX ERROR: Expected a valid expression term after assignment operator '<-'.\n");
                        exit(1);
                }
                else {
                printf("! SYNTAX ERROR: Expected assignment operator '<-' after non-function name identifier.\n") ;
                exit(EXIT_FAILURE);
                }     
            }
            break;
        
        case TknPrint:
            pMoveToNextTkn(); // eat print nom nom nom 
            stmtNode -> data.stmt.data.print.exp = pExpression();
            stmtNode -> type = nodePrint;

            // validate that expression exists
            if (!stmtNode->data.stmt.data.print.exp) {
                printf("! SYNTAX ERROR: Expected a valid expression after 'print'.\n");
                exit(EXIT_FAILURE);
            }
            break;

        case TknReturn:
            pMoveToNextTkn();
            stmtNode->data.stmt.data.returnStmt.exp = pExpression();
            stmtNode->type = nodeReturn;
            
            // Validate that the expression is valid
            if (!stmtNode->data.returnStmt.exp) {
                printf("! SYNTAX ERROR: Expected a valid expression after 'return'.\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            // error rip
            printf("! SYNTAX ERROR: Unexpected token. valid statement starting args include print, return and function calls.");
            exit(EXIT_FAILURE);
    }
    return stmtNode;
}

#define MAX_PARAMS 100
#define MAX_STATEMENTS 1000
// parsing over a function definition
AstNode* pFuncDef() {
    // add node to tree
    AstNode* funcDefNode = createNode(nodeFunctionDef);
    pMoveToNextTkn();  // Go to next token

    // check if func exists
    if (pCurrentTkn().type == TknIdentifier) {
        if (doesFunctionExist(pCurrentTkn().value)) {  // check for if function already exists
            printf("! SYNTAX ERROR: Function name '%s' already definied\n", pCurrentTkn().value);
            exit(EXIT_FAILURE);
        }

        // otherwise add to func list
        addFunctionName(pCurrentTkn().value); // Store function name
        funcDefNode->data.funcDef.identifier = strdup(pCurrentTkn().value); // Function name
        funcDefNode->data.funcDef.params = (char**)malloc(sizeof(char*) * MAX_PARAMS); // CHECK THIS PLEASE
        funcDefNode->data.funcDef.paramCount = 0; // initalise to 0
        pMoveToNextTkn();
            
        // Expecting open bracket
        if (pCurrentTkn().type == TknLBracket) {
            pMoveToNextTkn(); // move past (
            
            while (1) {
                if (pCurrentTkn().type == TknIdentifier) {
                    if (funcDefNode -> data.funcDef.paramCount >= MAX_PARAMS) {
                        printf("! SYNTAX ERROR: Too many params in func def, maximum allowed is %d.\n", MAX_PARAMS);
                        exit(EXIT_FAILURE);
                    }

                // add identifier to list of parameters
                funcDefNode->data.funcDef.params[funcDefNode->data.funcDef.paramCount++] = strdup(pCurrentTkn().value);
                pMoveToNextTkn();  
                    
                    if (pCurrentTkn().type == TknComma) { // expect comma
                        pMoveToNextTkn();
                    }
                    else if (pCurrentTkn().type == TknRBracket) {
                        pMoveToNextTkn(); // function parameter list ends 
                        break; // continues after while loop broken
                    } 
                    else {
                        printf("! SYNTAX ERROR: Expected ',' or ')' after function argument\n");
                        exit(EXIT_FAILURE);
                    }  
                } 
                else {
                    // Handle unexpected tokens
                    printf("! SYNTAX ERROR: Expected identifier for function parameter, but got '%s'\n", pCurrentTkn().value);
                    exit(EXIT_FAILURE);
                }
            }
        } 
        else if (pCurrentTkn().type == TknNewline) {
            // valid function with no parameters do nothing
        } 
        else {
            printf("! SYNTAX ERROR: Expected newline for empty function after function parameters\n");
            exit(EXIT_FAILURE);
        }

        // Parse the function body (statements)
        funcDefNode->data.funcDef.stmt = (AstNode**)malloc(sizeof(AstNode*) * MAX_STATEMENTS); 
        funcDefNode->data.funcDef.stmtCount = 0;

        while (pCurrentTkn().type == TknTab) { // Expecting indented statements
            if (funcDefNode->data.funcDef.stmtCount < MAX_STATEMENTS) {
                funcDefNode->data.funcDef.stmt[funcDefNode->data.funcDef.stmtCount++] = pStmt(); // Parse statement
            } else {
                printf("@ Error: Too many statements in function body. Maximum is %d.\n", MAX_STATEMENTS);
                exit(EXIT_FAILURE);
            }

            if (pCurrentTkn().type == TknNewline) {
                pMoveToNextTkn(); // Move to next line
            } 
            else if (pCurrentTkn().type != TknTab) {
                break; // Exit loop if not indented anymore
            }
        }
    } 
    else {
        printf("! SYNTAX ERROR: Expected identifier for function name\n");
        exit(EXIT_FAILURE);
    }

    return funcDefNode; // Return the created function definition node
}

AstNode* pProgItem() {
    // handle newlines (skip and continue)
    while (pCurrentTkn().type == TknNewline) {
        pMoveToNextTkn();
    }

    // check for if function definition exists
    if (pCurrentTkn().type == TknFunction) {
        return pFuncDef();
    } 
    else if (pCurrentTkn().type == TknIdentifier 
    || pCurrentTkn().type == TknPrint 
    || pCurrentTkn().type == TknReturn) {
        AstNode* stmtNode = pStmt();
        
        // Expect newline or end after each statement
        if (pCurrentTkn().type == TknNewline) {
            pMoveToNextTkn();  // Consume newline or end
        } else if (pCurrentTkn().type == TknEnd) {
            return stmtNode;  
        } 
        else {
            printf("! SYNTAX ERROR: Expected newline or end after statement, got '%s'.\n", pCurrentTkn().value);
            exit(EXIT_FAILURE);
        }
        return stmtNode;
    } else if (pCurrentTkn().type == TknEnd) {
        // End of program, no further tokens expected
        return NULL;
    } else {
        // handle unexpected tokens
        printf("! SYNTAX ERROR: Unexpected token '%s'. Expected function definition or statement.\n", pCurrentTkn().value);
        exit(EXIT_FAILURE);
    }
}

#define MAX_LINES 1000
AstNode* pProgram() {
    AstNode* programNode = createNode(nodeProgram);
    
    // line count
    programNode -> data.program.lineCount = 0;
    programNode->data.program.programItems = (AstNode**)malloc(sizeof(AstNode*) * MAX_LINES); // CHECK THIS PLEASE
    if (!programNode -> data.program.programItems) {
        fprintf(stderr, "Memory alloc error.");
        exit(EXIT_FAILURE);
    }
    // parsing over program
    while (pCurrentTkn().type != TknEnd) {
        AstNode* programItem = pProgItem();
        if (programItem != NULL) {
            if (programNode -> data.program.lineCount < MAX_LINES) {
                programNode -> data.program.programItems[programNode->data.program.lineCount++] = programItem;
            } else {
                printf("! SYNTAX ERROR: Maximum line count exceeded.");
                exit(EXIT_FAILURE);
            }
        }
        pMoveToNextTkn();
    }
    return programNode;
}

// ------------------- TESTING -------------------

const char *testCode = 
    "function myFunction() {\n"
    "    print 123;\n"
    "    return 456.78;\n"
    "}\n";

void testLexer() {
    printf("Testing Lexer:\n");
    
    // Tokenize the test code
    tokenize(testCode);

    // Print all tokens (assuming you have a way to access Tokens array)
    for (int i = 0; i < TknCount; i++) {
        print_token(Tokens[i]); // Make sure you have a print function for tokens
    }
}

void runTest(const char* testCode) {
    // Set up your tokenizer with the test code here
    // Tokenize the input, store in `Tokens`, reset `pCurrentTknIndex`, etc.

    AstNode* result = pProgram(); // Run the parser
    if (result != NULL) {
        printf("Test passed!\n");
    } else {
        printf("Test failed!\n");
    }

    // Clean up memory, if necessary
}

void parseFile(const char* filename) {
    int result = readFile(filename);
    if (result == 0) {
        pProgram();
        // Print the parsed program (optional)
        // printProgram(program);
    }
}

int main() {
    testLexer();
    return 0;
}
