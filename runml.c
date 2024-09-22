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
    TknEnd, // "END" 
    TknArg // arg0, arg1, etc.
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
            int isReturn;
            bool hasOperators;

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
                    char* identifier;
                    struct AstNode **args;
                    int argCount;
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
            float constant; // using float as we only require 6 digits of prec
            char* identifier;
            char* argfunc;
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


#define MAX_FARGS 100

char *args[MAX_FARGS];

typedef struct {
    char *args[MAX_FARGS]; // Array to store unique args
    int count; // Number of unique args
} UniqueArgs;

void collectUniqueArgs(Token *tokens, int tokenCount, UniqueArgs *uniqueArgs) {
    uniqueArgs->count = 0; // Initialize count to 0

    for (int i = 0; i < tokenCount; i++) {
        if (tokens[i].type == TknArg) {
            // Check if the arg already exists in uniqueArgs
            int exists = 0;
            for (int j = 0; j < uniqueArgs->count; j++) {
                if (strcmp(uniqueArgs->args[j], tokens[i].value) == 0) {
                    exists = 1; // Arg already exists
                    break;
                }
            }
            // If it doesn't exist, add it to the uniqueArgs
            if (!exists && uniqueArgs->count < MAX_FARGS) {
                uniqueArgs->args[uniqueArgs->count++] = strdup(tokens[i].value);
            }
        }
    }
}

// ###################################### TOKENISATION START ######################################

// FOR TESTING PURPOSES - print the token
void print_token(Token token) {
    printf("Token Type: %d, Value: %s\n", token.type, token.value);
}


// initalise "Tokens" array
Token Tokens[1000]; // array that stores all tokens generated by the lexer
int TknIndex = 0; // integer value that keeps track of our current position in array
int TknCount = 0; // stores the number of tokens in our array
int argsCount = 0; // store number of args called

// function to add tokens to our "Tokens" array
void addToken(TknType type, const char *value) { 
    Tokens[TknIndex].type = type; // sets type
    strcpy(Tokens[TknIndex].value, value); // sets value 
    TknIndex++; // increases token index/position pointer
    TknCount++; // increment token count
    print_token(Tokens[TknIndex - 1]);
    printf("Creating Token - Type: %d, Value: '%s'\n", type, value); // debugging
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

void tokenize(const char *code) {
    const char *pointer = code; // accesses character in code
    int IndentLevel = 0; // integer to track indent level

    while (*pointer) {

        // check for comments first (to remove them from consideration and avoid errors later on)
        if (*pointer == '#') {

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
                TempBuffer[i++] = *pointer++; 
            } 
            TempBuffer[i] = '\0'; // Null-terminate the buffer

            // Check if the identifier starts with "arg"
            if (strncmp(TempBuffer, "arg", 3) == 0) {
            // After "arg", ensure the remaining characters are digits
                int isValidArg = 1;
                for (int j = 3; TempBuffer[j] != '\0'; j++) {
                    if (!isdigit(TempBuffer[j])) {
                        isValidArg = 0;
                        break;
                }
            }   
                if (isValidArg) {
            // If valid, create a TknArg token and store the full "argX" value
                    addToken(TknArg, TempBuffer); // Use the new token type for argX
                    argsCount++;
                } else {
                fprintf(stderr, "! Syntax Error: Invalid identifier after 'arg'.\n");
                exit(1);
                }
            }
            else if (strcmp(TempBuffer, "function") == 0) { // if function keyword exists
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
                        TknType = TknLBracket;
                        break;
                    case ')':
                        TknType = TknRBracket;  // if '(' and ')' (brackets) exists
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

    // At the end of input, add an end token
    addToken(TknEnd, "END"); // Add end token when reaching the end of the code

    return 0;
}

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
    if (pCurrentTknIndex < TknCount - 1) {
        pCurrentTknIndex++;
    }
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

bool hasOperatorInExpression(AstNode* node) {
    if (!node) return false;

    switch (node->type) {
        case nodeExpression:
        case nodeTerm:
            // If the expression or term has an operator, return true
            if (node->data.Expression.oper != NULL || node->data.term.oper != NULL) {
                return true;
            }

            // Recursively check the left and right variables of the expression/term
            if (hasOperatorInExpression(node->data.Expression.lVar) || 
                hasOperatorInExpression(node->data.Expression.rVar)) {
                return true;
            }
            break;

        case nodeFactor:
            // If it's a function call or nested expression, check that recursively
            if (node->data.factor.funcCall) {
                return hasOperatorInExpression(node->data.factor.funcCall);
            } else if (node->data.factor.exp) {
                return hasOperatorInExpression(node->data.factor.exp);
            }
            break;

        default:
            break;
    }

    return false;
}


AstNode* pFactor() {
    printf("Entering pFactor()\n");
    AstNode* factorNode = NULL;
    
    // check for number existence
    if (pCurrentTkn().type == TknNumber) {
        printf("Found number: %f\n", atof(pCurrentTkn().value));  // Use %f for double
        factorNode = createNode(nodeFactor);
        factorNode -> data.factor.constant = atof(pCurrentTkn().value);
        pMoveToNextTkn();
    }
    // check for float existence
    else if (pCurrentTkn().type == TknFloat) {
        printf("Found number: %f\n", atof(pCurrentTkn().value));  // Use %f for double
        factorNode = createNode(nodeFactor);
        printf("Created float node");
        factorNode -> data.factor.constant = atof(pCurrentTkn().value);
        pMoveToNextTkn();
    } 
    else if (pCurrentTkn().type == TknArg) {
        factorNode = createNode(nodeFactor);
        factorNode -> data.factor.argfunc = strdup(pCurrentTkn().value);
        printf("Stored identifier: %s\n", factorNode->data.factor.argfunc);
        pMoveToNextTkn();
    }
    else if (pCurrentTkn().type == TknIdentifier) {
        printf("Found identifier: %s\n", pCurrentTkn().value);
        // if function call
        if (doesFunctionExist(pCurrentTkn().value)) { 
            factorNode = createNode(nodeFactor);
            factorNode -> data.factor.funcCall = pFuncCall();
            // Allow endline or other tokens after function call
            if (pCurrentTkn().type == TknEnd) {
                printf("Found endline after function call\n");
            }
        }
        //not function call
        else    {
            factorNode = createNode(nodeFactor);
            factorNode -> data.factor.identifier = strdup(pCurrentTkn().value);
            printf("Stored identifier: %s\n", factorNode->data.factor.identifier);
            pMoveToNextTkn();
            //if (pCurrentTkn().type != TknNewline && pCurrentTkn().type != TknEnd) {
            //    printf ("! SYNTAX ERROR: Expected new line after non-function name identifier\n.");
            //    exit(1);
            //}
        }
    } else if (pCurrentTkn().type == TknLBracket) {
        printf("Found left bracket\n");
        pMoveToNextTkn();
        factorNode = createNode(nodeFactor);
        // check stuff within the brackets 
        factorNode -> data.factor.exp = pExpression();
            if(pCurrentTkn().type != TknRBracket) {
                printf("! SYNTAX ERROR: Invalid factor. Expected ')' after expression.\n");
                exit(1);
            }    
        printf("Found right bracket\n");
        pMoveToNextTkn(); // consume ')
        return factorNode;
        
    } 
    else {
        printf(" TOKEN : '%s' (Type: %d)\n", pCurrentTkn().value, pCurrentTkn().type);
        printf("! SYNTAX ERROR: Invalid factor. Expected functioncall, real constant, identifer or '(' expression ')'.\n.");
        exit(1);
    }
    printf("Exiting pFactor()\n");
    return factorNode;
}

// creating that left right operator child tree
AstNode* pTerm(){
    printf("Entering pTerm()\n");    
    AstNode* fctrNode = pFactor(); // parse first factor
    // if is factor operator must parse recursively

    // debug
    if (!fctrNode) {
        printf("! SYNTAX ERROR: Expected a valid factor.\n");
        return NULL; // Handle error
    }

    printf("Parsed factor: %f\n", fctrNode->data.factor.constant); 

    while (pCurrentTkn().type == TknFactorOperator) {
        char* oper = strdup(pCurrentTkn().value); // store oper
        printf("Found multiplication/division operator: '%s'\n", oper);


        pMoveToNextTkn(); // move to next token
        AstNode* rVarNode = pTerm(); // parse next token

        // debug
        if (!rVarNode) {
            printf("! SYNTAX ERROR: Expected valid factor after operator '%s'.\n", oper);
            free(oper); // Clean up
            return NULL; // Handle error
        }

        AstNode* termNode = createNode(nodeTerm);
        termNode -> data.term.lVar = fctrNode; // node given lVar property i.e. the left factor 
        termNode -> data.term.rVar = rVarNode; // right variable 
        termNode -> data.term.oper = oper; // operator assigned as well
        fctrNode = termNode; 
    } 
    printf("Exiting pTerm()\n");
    return fctrNode;
}

AstNode* pExpression(){
    printf("Entering pExpression() with current node %s\n", pCurrentTkn().value);
    AstNode* termNode = pTerm(); // parse first term
    if (!termNode) {
        printf("! SYNTAX ERROR: Expected a valid term.\n");
        return NULL; // Return or handle error
    }
    
    while (pCurrentTkn().type == TknTermOperator) {
        char* oper = strdup(pCurrentTkn().value);
        printf("Found term operator: '%s'\n", oper);

        pMoveToNextTkn();
        AstNode* rVarNode = pExpression();

        // debug
        if (!rVarNode) {
            printf("! SYNTAX ERROR: Expected valid expression after operator '%s'.\n", oper);
            free(oper); // Clean up
            return NULL; // Handle error
        }

        AstNode* exprNode = createNode(nodeExpression);
        exprNode -> data.Expression.lVar = termNode; 
        exprNode -> data.Expression.oper = oper;
        exprNode->data.Expression.rVar = rVarNode;
        termNode = exprNode;
    
    } 
    printf("Exiting pExpression()\n");
    return termNode;
}

#define MAX_ARGS 1000
AstNode* pFuncCall() {
    printf("Entering pFuncCall()\n");
    AstNode* funcCallNode = createNode(nodeFunctionCall);

    // Consume (EDIT: STORE) the function name
    if (pCurrentTkn().type == TknLBracket) {
    funcCallNode -> data.funcCall.identifier = strdup(Tokens[pCurrentTknIndex -1].value);
    printf("FumcCall: %s\n", funcCallNode->data.funcCall.identifier);

    }
    else {
    funcCallNode -> data.funcCall.identifier = strdup(Tokens[pCurrentTknIndex].value);
    printf("FumcCall: %s\n", funcCallNode->data.funcCall.identifier);
    }

    pMoveToNextTkn(); // function identifier eaten

    printf("Token currently: '%s' (Type: %d)\n", pCurrentTkn().value, pCurrentTkn().type);
    
    // throwing errors so lets do some malloc bullcrap
    funcCallNode -> data.funcCall.args = malloc(sizeof(AstNode*) * MAX_ARGS);
    funcCallNode -> data.funcCall.argCount = 0;

    // Check for the left bracket
    if (pCurrentTkn().type == TknLBracket) {
        pMoveToNextTkn();  // Consume '('
    }
    else {
        printf("! SYNTAX ERROR: Expected '(' after functioncall.\n");
        exit(1);
    }
    
    while (1) {
    if (pCurrentTkn().type == TknRBracket) {
        break;  // End of arguments
    } else if (pCurrentTkn().type == TknEnd || pCurrentTkn().type == TknNewline) {
        printf("! WARNING: Unexpected end or newline in function call arguments.\n");
        break;  // Exit early if we hit an end or newline
    }

    // Debugging output to check current token
    printf("Current token before pExpression: %s\n", Tokens[pCurrentTknIndex].value);

    // Parse the expression
    AstNode* paramNode = pExpression();
    if (paramNode) {
        funcCallNode->data.funcCall.args[funcCallNode->data.funcCall.argCount++] = paramNode;
    } else {
        printf("! SYNTAX ERROR: Invalid factor. Expected valid expression.\n");
        exit(1);
    }

    // Check for additional parameters
    if (pCurrentTkn().type == TknComma) {
        pMoveToNextTkn();  // Consume ','
    } else if (pCurrentTkn().type != TknRBracket) {
        printf("! SYNTAX ERROR: Expected ',' or ')' in function call arguments.\n");
        exit(1);
    }
}

    printf("Token after ID: '%s' (Type: %d)\n", pCurrentTkn().value, pCurrentTkn().type);

    // Check for the right parenthesis ')'
    if (pCurrentTkn().type == TknRBracket) {
         pMoveToNextTkn();  // Consume ')'} 
        printf("! DEBUGGING SHIT: Token after ID: '%s' (Type: %d)\n", pCurrentTkn().value, pCurrentTkn().type);
        return funcCallNode;
        
    } else {
        printf("! SYNTAX ERROR: Expected ')' after function parameters.\n");
        exit(1);
    }
}

#define MAX_VARIABLES 50

char variableNames[MAX_VARIABLES][12];
int variableCount = 0;

// Function to add a variable name
void addVariable(const char* name) {
    if (variableCount < MAX_VARIABLES) {
        strncpy(variableNames[variableCount++], name, 12);
    }
}

// parsing over statements
AstNode* pStmt() {
    printf("Entering pStmt() with node of type %d\n", pCurrentTkn().type);
    AstNode* stmtNode = createNode(nodeStmt);
    
    switch (pCurrentTkn().type) {
        case TknIdentifier:
            printf("Detected identifier: '%s'\n", pCurrentTkn().value); // debug
            
            if (doesFunctionExist(pCurrentTkn().value)) {
                printf("Function call detected for: '%s'\n", pCurrentTkn().value); // debug
                stmtNode -> data.stmt.data.funcCall;
                stmtNode->data.stmt.data.funcCall.identifier = strdup(pCurrentTkn().value);
                pMoveToNextTkn(); // consume identifier

//identical to function call but need repeat for reasons
                
                // throwing errors so lets do some malloc bullcrap
                stmtNode -> data.stmt.data.funcCall.args = malloc(sizeof(AstNode*) * MAX_ARGS);
                stmtNode -> data.stmt.data.funcCall.argCount = 0;

                // Check for the left bracket
                if (pCurrentTkn().type == TknLBracket) {
                    pMoveToNextTkn();  // Consume '('
                }
                else {
                    printf("! SYNTAX ERROR: Expected '(' after functioncall.\n");
                    exit(1);
                }

                while (1) {
                    if (pCurrentTkn().type == TknRBracket) {
                     break;  // End of arguments
                } else if (pCurrentTkn().type == TknEnd || pCurrentTkn().type == TknNewline) {
                    printf("! WARNING: Unexpected end or newline in function call arguments.\n");
                    break;  // Exit early if we hit an end or newline
                }

                // Debugging output to check current token
                printf("INSIDE Current token before pExpression: %s\n", Tokens[pCurrentTknIndex].value);


                // Parse the expression
                AstNode* paramNode = pExpression();
                if (paramNode) {
                    stmtNode->data.stmt.data.funcCall.args[stmtNode->data.stmt.data.funcCall.argCount++] = paramNode;
                } else {
                     printf("! SYNTAX ERROR: Invalid factor. Expected valid expression.\n");
                     exit(1);
                }

                // Check for additional parameters
                if (pCurrentTkn().type == TknComma) {
                    pMoveToNextTkn();  // Consume ','
                } else if (pCurrentTkn().type != TknRBracket) {
                    printf("! SYNTAX ERROR: Expected ',' or ')' in function call arguments.\n");
                    exit(1);
                }
                }

                printf("INSIDE Token after ID: '%s' (Type: %d)\n", pCurrentTkn().value, pCurrentTkn().type);

                // Check for the right parenthesis ')'
                if (pCurrentTkn().type == TknRBracket) {
                    pMoveToNextTkn();  // Consume ')'} 
                    printf("! INSIDE DEBUGGING SHIT: Token after ID: '%s' (Type: %d)\n", pCurrentTkn().value, pCurrentTkn().type);
        
                } else {
                    printf("! SYNTAX ERROR: Expected ')' after function parameters.\n");
                    exit(1);
                }
    /// identical code to function caller but need it for reasons 

                stmtNode->type = nodeFunctionCall;
                break;
            } else {
                printf("Assignment detected for: '%s'\n", pCurrentTkn().value); // debug
                // assignment
                addVariable(pCurrentTkn().value);
                stmtNode -> data.stmt.data.assignment.identifier = strdup(pCurrentTkn().value); // store identifier
                pMoveToNextTkn(); // move to next token
                printf("Current token after identifier: '%s' (Type: %d)\n", pCurrentTkn().value, pCurrentTkn().type);
                
                //check assignment operator correctly exists here
                if (pCurrentTkn().type == TknAssignmentOperator) {
                    printf("Assignment operator '<-' detected.\n");
                    pMoveToNextTkn(); // consume the assignment operator
                    
                    stmtNode -> data.stmt.data.assignment.exp = pExpression();
                    stmtNode -> type = nodeAssignment;
                
                    // validate expression exists for assignment operator 
                    if (!stmtNode->data.stmt.data.assignment.exp) {
                        printf("! SYNTAX ERROR: Expected a valid expression term after assignment operator '<-'.\n");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    printf("! SYNTAX ERROR: Expected assignment operator '<-' after non-function name identifier.\n") ;
                    exit(EXIT_FAILURE);
                }     
                break;
            }

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
            if (!stmtNode->data.stmt.data.returnStmt.exp) {
                printf("! SYNTAX ERROR: Expected a valid expression after 'return'.\n");
                exit(EXIT_FAILURE);
            }
            break;
        default:
            // error rip
            printf("! SYNTAX ERROR: Unexpected token. valid statement starting args include print, return and function calls.");
            exit(EXIT_FAILURE);
    }

    printf("Exiting pStmt()\n");
    return stmtNode;
}

#define MAX_PARAMS 100
#define MAX_STATEMENTS 1000
// parsing over a function definition
AstNode* pFuncDef() {
    printf("Entering pFuncDef()\n");

    // Create a new function definition node
    AstNode* funcDefNode = createNode(nodeFunctionDef);
    pMoveToNextTkn();  // Move to the next token after 'function' keyword

    // Check if a valid function name (identifier) is provided
    if (pCurrentTkn().type == TknIdentifier) {
        if (doesFunctionExist(pCurrentTkn().value)) {
            printf("! SYNTAX ERROR: Function name '%s' is already defined\n", pCurrentTkn().value);
            exit(EXIT_FAILURE);
        }

        // Store function name in the function definition node
        addFunctionName(pCurrentTkn().value);
        funcDefNode->data.funcDef.identifier = strdup(pCurrentTkn().value);

        // Allocate memory for function parameters
        funcDefNode->data.funcDef.params = (char**)malloc(sizeof(char*) * MAX_PARAMS);
        funcDefNode->data.funcDef.paramCount = 0;
        pMoveToNextTkn();  // Move to parameters

        // Parse the function parameters (if any)
        while (pCurrentTkn().type == TknIdentifier) {
            if (funcDefNode->data.funcDef.paramCount >= MAX_PARAMS) {
                printf("! SYNTAX ERROR: Too many parameters in function definition\n");
                exit(EXIT_FAILURE);
            }

            // Add the identifier as a parameter to the function
            funcDefNode->data.funcDef.params[funcDefNode->data.funcDef.paramCount++] = strdup(pCurrentTkn().value);
            pMoveToNextTkn();  // Move to the next parameter
        }

        // Ensure there is a newline after the function name and parameters
        if (pCurrentTkn().type != TknNewline) {
            printf("! SYNTAX ERROR: Expected newline after function definition\n");
            exit(EXIT_FAILURE);
        }
        pMoveToNextTkn();  // Move past the newline

        // Parse the function body, which consists of indented statements
        funcDefNode->data.funcDef.stmt = (AstNode**)malloc(sizeof(AstNode*) * MAX_STATEMENTS);
        funcDefNode->data.funcDef.stmtCount = 0;
        funcDefNode->data.funcDef.isReturn = 0;  // Initialize to indicate no return statement yet

        // Parse the statements inside the function body
        while (pCurrentTkn().type == TknTab) {
            pMoveToNextTkn();  // Move past the tab (indentation)

            if (funcDefNode->data.funcDef.stmtCount >= MAX_STATEMENTS) {
                printf("! SYNTAX ERROR: Too many statements in function body\n");
                exit(EXIT_FAILURE);
            }

            // Parse an individual statement and add it to the function's statement list
            AstNode* stmtNode = pStmt();  // Parse a statement specifically for the function
            funcDefNode->data.funcDef.stmt[funcDefNode->data.funcDef.stmtCount++] = stmtNode;
            funcDefNode->data.funcDef.stmtCount++;

            // Check if the parsed statement is a return statement
            if (stmtNode->type == nodeReturn) {
                funcDefNode->data.funcDef.isReturn = 1;
            }

            // Move to the next token after the statement
            if (pCurrentTkn().type == TknNewline) {
                pMoveToNextTkn();  // Move to the next line
            } else {
                break;  // Stop if there are no more indented statements
            }
        }

        // Make sure that the function body contains at least one statement
        if (funcDefNode->data.funcDef.stmtCount == 0) {
            printf("! SYNTAX ERROR: Function body must contain at least one statement\n");
            exit(EXIT_FAILURE);
        }
    } else {
        printf("! SYNTAX ERROR: Expected identifier for function name\n");
        exit(EXIT_FAILURE);
    }

    return funcDefNode;  // Return the created function definition node
}


AstNode* pProgItem() {
    printf("Entering pProgItem()\n");
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
            return stmtNode;
        } else if (pCurrentTkn().type == TknEnd) {
            return stmtNode;  
        } else {
            printf("! SYNTAX ERROR: Expected newline or end after statement, got '%s'.\n", pCurrentTkn().value);
            exit(EXIT_FAILURE);
        }
        return stmtNode;
    } else if (pCurrentTkn().type == TknEnd) {
        // End of program, no further tokens expected
        return NULL;
    } else if (pCurrentTkn().type == TknTab) {
            printf("! Detected Tab as start of program item");
            exit(EXIT_FAILURE);
    } else {
        // handle unexpected tokens
        printf("! SYNTAX ERROR: Unexpected token '%s'. Expected function definition or statement.\n", pCurrentTkn().value);
        exit(EXIT_FAILURE);
    }
}

#define MAX_LINES 10000
AstNode* pProgram() {
    printf("Entering pProgram()\n");
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
    }
    return programNode;
}

// ------------------------------------------- INTERPRETER-------------------------------------- //

//declare interpreter buffer size
#define BUFFER_SIZE 10000 // may change

char* codeBuffer; // Global buffer for C code
int bufferLength = 0;

// Free the buffer
void freeBuffer() {
    free(codeBuffer);
}

// Initialize buffer
void initBuffer() {
    codeBuffer = malloc(BUFFER_SIZE); // CHECK THIS PLEASE
    if (!codeBuffer) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    codeBuffer[0] = '\0'; // Start with an empty string
}

/// Append string to buffer
void addToCodeBuffer(const char* str) {
    int len = strlen(str);
    if (bufferLength + len >= BUFFER_SIZE) {
        // Resize buffer if necessary
        char* newBuffer = realloc(codeBuffer, bufferLength + len + 1);
        if (!newBuffer) {
            fprintf(stderr, "Memory reallocation failed\n");
            freeBuffer(); // Clean up existing buffer
            exit(1);
        }
        codeBuffer = newBuffer;
    }
    strcat(codeBuffer, str);
    bufferLength += len;
}

const char* getExpStr(AstNode* expr) {
    static char buffer[100]; 
    buffer[0] = '\0'; 

    if (!expr) return buffer;

    switch (expr->type) {
        case nodeFactor:
            snprintf(buffer, sizeof(buffer), "%s", expr->data.factor.identifier); // Adjust as needed
            break;
        case nodeExpression:
            snprintf(buffer, sizeof(buffer), "%s %s %s",
                getExpStr(expr->data.Expression.lVar),
                expr->data.Expression.oper,
                getExpStr(expr->data.Expression.rVar));
            break;
        default:
            break;
    }
    printf("Generated expression: %s\n", buffer); 
    return buffer;
}


void writeCFile() {
    FILE *cFile = fopen("mlProgram.c", "w");
    if (cFile == NULL) {
        fprintf(stderr, "! Error: Could not create C file\n");
        return;
    }
    fprintf(cFile, "%s", codeBuffer); // buffer to file
    fclose(cFile);
}

bool containsFunctionCall(AstNode* node) {
    if (!node) return false;

    // Check if the current node is a function call
    if (node->type == nodeFunctionCall) {
        return true;
    }

    // Check if it's an expression or term and recursively check its components
    switch (node->type) {
        case nodeExpression:
            return containsFunctionCall(node->data.Expression.lVar) || 
                   containsFunctionCall(node->data.Expression.rVar);
        case nodeTerm:
            return containsFunctionCall(node->data.term.lVar) || 
                   containsFunctionCall(node->data.term.rVar);
        case nodeFactor:
            if (node->data.factor.funcCall) {
                return true; // This node contains a function call
            }
            return containsFunctionCall(node->data.factor.exp);
        default:
            return false; // No function call found in this node
    }
}

// defining translation to rudimentaty C program
void toC(AstNode* node) {

    // first we gotta check if the node is existing
    if (!node) {
        return;
    }
    switch (node->type) {
        case nodeProgram:
            addToCodeBuffer("#include <stdio.h>\n\n");

            // Generate variable declarations
            for (int i = 0; i < variableCount; i++) {
                addToCodeBuffer("AssiType ");
                addToCodeBuffer(variableNames[i]);
                addToCodeBuffer(";\n");
            }

            // Flag to check if funcdef exists
            bool functionDefined = false;
            // int storedI = 0;
            // First pass to collect function definitions
            for (int i = 0; i < node->data.program.lineCount; i++) {
                if (node->data.program.programItems[i]->type == nodeAssignment && !functionDefined) { // handle global variable
                    addToCodeBuffer("AssiType "); // to do
                    addToCodeBuffer(node->data.program.programItems[i]->data.stmt.data.assignment.identifier);
                    addToCodeBuffer(" = ");
                    toC(node->data.program.programItems[i]->data.stmt.data.assignment.exp);
                    addToCodeBuffer(";\n");
                }
                else if (node->data.program.programItems[i]->type == nodeFunctionDef) {
                    functionDefined = true;
                    toC(node->data.program.programItems[i]);
  
                }
            }

            // add main functions
             addToCodeBuffer("int main(int argc, char *argv[]) {\n");

            char arglinebuffer[256]; // Adjust size as needed


            // Check if there are any arguments to include
            if (argsCount > 0) {
                printf("Error: program cannot handle arguments :( \n");
                exit(EXIT_FAILURE);
            }
            
            bool hasReturn = false; // flag to track if a return statement is made in main

            // Process all statements that should be executed in main
for (int j = 0; j < node->data.program.lineCount; j++) {
    if (node->data.program.programItems[j]->type == nodePrint) { // Handle print statements
        toC(node->data.program.programItems[j]);
    } else if (node->data.program.programItems[j]->type == nodeAssignment) { // Handle assignments
        toC(node->data.program.programItems[j]);
    } else if (node->data.program.programItems[j]->type == nodeReturn) { // Handle return statements
        toC(node->data.program.programItems[j]);
        hasReturn = true;
    } else if (node->data.program.programItems[j]->type == nodeFunctionCall) { // Handle function calls in main
        printf("detected functioncall in stmt interpreter");
        toC(node->data.program.programItems[j]);
    }
}
            // Only add return 0 if no return statement has been encountered
            if (!hasReturn) {
                addToCodeBuffer("    return 0;\n");
            }
            addToCodeBuffer("}\n");
            break;
        
        case nodeFunctionDef:
            if (node->data.funcDef.isReturn == 1) {
                addToCodeBuffer("int ");
            }
            else if (node->data.funcDef.isReturn == 0) {
                addToCodeBuffer("void ");
            }
            else { 
                fprintf(stderr, "IDK what the fuck happened here\n");
                exit(1);
            }
            addToCodeBuffer(node->data.funcDef.identifier);
            addToCodeBuffer("(");

            for (int i = 0; i < node->data.funcDef.paramCount; i++) {
                if (i > 0) addToCodeBuffer(", "); // need to change
                    addToCodeBuffer("int ");
                    addToCodeBuffer(node->data.funcDef.params[i]);
            }
            addToCodeBuffer(") {\n");

            // Add the function body
            for (int j = 0; j < node->data.funcDef.stmtCount; j++) {
                toC(node->data.funcDef.stmt[j]);
            }
            addToCodeBuffer("}\n\n");
            break;

        case nodeAssignment:
            addToCodeBuffer("AssiType ");
            addToCodeBuffer(node->data.stmt.data.assignment.identifier);
            addToCodeBuffer(" = ");
            toC(node->data.assignment.exp);
            addToCodeBuffer(";\n");
            break;

        case nodePrint:
            addToCodeBuffer("printf(");

            // Check if the expression contains a function call
            if (containsFunctionCall(node->data.stmt.data.print.exp)) {
                // You can determine the format based on your specific rules here
                addToCodeBuffer("\"%d\\n\"");  // we can assume that if a functioncall exists, the function performs a mathematical operation
            } else {
                // Regular expression handling
                if (hasOperatorInExpression(node->data.stmt.data.print.exp)) {
                    addToCodeBuffer("\"%d\\n\"");  // Use integer format if there's an operator
                } else {
                    addToCodeBuffer("\"%f\\n\"");  // Use floating-point format if it's a single value
                }
            }

            addToCodeBuffer(", ");

            // Output the full expression to the buffer
            toC(node->data.stmt.data.print.exp);
    
            addToCodeBuffer(");\n");
             break;

        case nodeReturn:
            addToCodeBuffer("return ");
            toC(node->data.stmt.data.returnStmt.exp);
            addToCodeBuffer(";\n");
            break;

        case nodeExpression:
            // Process the left side of the term
            toC(node->data.Expression.lVar);  // Left-hand side of the term (could be a factor)
    
            // Add the operator (if any)
            if (node->data.Expression.oper != NULL) {
                addToCodeBuffer(node->data.Expression.oper);  // The operator (like *, /)
            }
    
            // Process the right side of the term (if there is one)
            if (node->data.Expression.rVar != NULL) {
                toC(node->data.Expression.rVar);  // Right-hand side of the term
            }
    
            break;

        case nodeTerm:
            // Process the left side of the term
            toC(node->data.term.lVar);  // Left-hand side of the term (could be a factor)
    
            // Add the operator (if any)
            if (node->data.term.oper != NULL) {
                addToCodeBuffer(node->data.term.oper);  // The operator (like *, /)
            }
    
            // Process the right side of the term (if there is one)
            if (node->data.term.rVar != NULL) {
                toC(node->data.term.rVar);  // Right-hand side of the term
            }
    
            break;

        case nodeFunctionCall: 
            if(node->data.funcCall.identifier == NULL) { // statement function call exists here
                addToCodeBuffer(node->data.stmt.data.funcCall.identifier);
                addToCodeBuffer("(");
                for (int i = 0; i < node->data.stmt.data.funcCall.argCount; i++) {
                    if (i > 0) {
                        addToCodeBuffer(",");
                }
                toC(node->data.stmt.data.funcCall.args[i]);
                }
                 addToCodeBuffer(");\n"); 
                //}
                break;
            }
            else {
            addToCodeBuffer(node->data.funcCall.identifier);
            addToCodeBuffer("(");
            for (int i = 0; i < node->data.funcCall.argCount; i++) {
                if (i > 0) {
                    addToCodeBuffer(",");
                }
                toC(node->data.funcCall.args[i]);
            }
            addToCodeBuffer(")"); 
            //}
            break;
            }

        case nodeFactor:
            if (node->data.factor.identifier) { // identifier exists
                addToCodeBuffer(node->data.factor.identifier);
            }
            else if (node->data.factor.argfunc) {
                // Assuming argfunc represents an argument variable (e.g., arg0, arg1, etc.)
                const char *argFunc = node->data.factor.argfunc;

            // Check if argFunc matches the pattern "argX" where X is a digit
                if (strncmp(argFunc, "arg", 3) == 0) {
                    int argIndex = atoi(argFunc + 3); // Get index from arg0, arg1, etc.
                    addToCodeBuffer("argv[");
                    char buffer[10];
                    snprintf(buffer, sizeof(buffer), "%d", argIndex + 1); // Adjust for argv indexing
                    addToCodeBuffer(buffer);
                    addToCodeBuffer("]");
                } else {
                    addToCodeBuffer(argFunc); // Handle any other cases as necessary
                }
            }
            else if (node->data.factor.funcCall) { // else if function call exists
                toC(node->data.factor.funcCall);
            }
            else if (node->data.factor.exp) { // if expression exists
                toC(node->data.factor.exp);
            }
            else { // otherwise treat as constant
                char buffer[50]; // Adjust size as needed
                snprintf(buffer, sizeof(buffer), "%.6f", node->data.factor.constant);
                addToCodeBuffer(buffer);
            }
            break;

        default:
            // Handle error or unsupported node types
            fprintf(stderr, "unknown AST node type: %d\n", node->type);
    }
}

// ---------------------------------- REPLACING ASSITYPE ------------------------------//
typedef struct {
    char* name;
    int operated; // 1 (true) and 0 (false)
} VarInf;
int isVarFound(const char* line, const char* variable) {
    const char* pos = line;
    size_t varLen = strlen(variable);
    while ((pos = strstr(pos, variable))) {
        char before = (pos == line) ? ' ' : *(pos - 1);
        char after = *(pos + varLen);
        if ((before == ' ' || before == '=' || before == '(' || before == ',' || before == ';' || before == '\0') &&
            (after == ' ' || after == '=' || after == ')' || after == ',' || after == ';' || after == '\0')) {
            return 1; 
            }
        pos += varLen;
        }
    return 0; 
}
int findAssi(const char* buffer, VarInf vars[], int* varCount) {
    char* lines[MAX_LINES];
    int lineCount = 0;
    
    char* bufferCopy = strdup(buffer);
    if (!bufferCopy) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }
    char* line = strtok(bufferCopy, "\n");
    while (line) {
        lines[lineCount++] = line;
        line = strtok(NULL, "\n");
    }
    for (int i = 0; i < lineCount; i++) {
        char* pos = strstr(lines[i], "AssiType ");
        if (pos && (strchr(pos, ';') != NULL)) {
            char* varName = malloc(13); // 12 letters + whitespace
            sscanf(pos, "AssiType %12[^; ]", varName);
            
            vars[*varCount].name = varName;
            vars[*varCount].operated = 0; 
            (*varCount)++;
        }
    }
    free(bufferCopy);
    return lineCount;
}
void checkVarPres(const char* buffer, VarInf vars[], int varCount, int lineCount) {
    char* lines[MAX_LINES];
    char* bufferCopy = strdup(buffer);
    if (!bufferCopy) {
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    char* line = strtok(bufferCopy, "\n");
    int i = 0;
    while (line) {
        lines[i++] = line;
        line = strtok(NULL, "\n");
    }
    for (int v = 0; v < varCount; v++) {
        for (int i = 0; i < lineCount; i++) {
            if (isVarFound(lines[i], vars[v].name)) {
                if (strstr(lines[i], "=") || strstr(lines[i], "+") || strstr(lines[i], "-") || 
                    strstr(lines[i], "*") || strstr(lines[i], "/")) {
                    vars[v].operated = 1; 
                }
            }
        }
    }
    free(bufferCopy);
}
void replAssi(const char* buffer, VarInf vars[], int varCount, char* outputBuffer) {
    strcpy(outputBuffer, buffer); 
    for (int v = 0; v < varCount; v++) {
        if (vars[v].operated) {
            char oldString[BUFFER_SIZE];
            sprintf(oldString, "AssiType %s", vars[v].name);
            char newString[BUFFER_SIZE];
            sprintf(newString, "int %s", vars[v].name);
            char* pos = outputBuffer;
            while ((pos = strstr(pos, oldString))) {
                strncpy(pos, newString, strlen(newString));
                memmove(pos + strlen(newString), pos + strlen(oldString), strlen(pos + strlen(oldString)) + 1);
                pos += strlen(newString); 
            }
        }
    }
}

char outputBuffer[BUFFER_SIZE];

void conductAssiReplace(const char* buffer) {
    VarInf vars[MAX_LINES];
    int varCount = 0;
    int lineCount = findAssi(buffer, vars, &varCount);
    checkVarPres(buffer, vars, varCount, lineCount);
    replAssi(buffer, vars, varCount, outputBuffer);
    
    printf("%s\n", outputBuffer); 
    
    for (int i = 0; i < varCount; i++) {
        free(vars[i].name);
    }
}

// ###################################### RUNNING C PROGRAM START ######################################

void compileAndRunInC() {
    system("gcc -o mlProgram mlProgram.c");
    system("mlProgram.exe"); // Run the executable
}

// function to remove created C file and exec file
void cleanupAfterExec() {
    remove("mlProgram.c");
    remove("mlProgram");
}


double commandLineArgs[MAX_ARGS];  // Array to hold real-valued arguments

void parseCommandLineArgs(int argc, char *argv[]) {
    for (int i = 0; i < argc - 1 && i < MAX_ARGS; i++) {
        commandLineArgs[i] = atof(argv[i + 1]);  // Convert argument to float
        printf("Argument arg%d: %f\n", i, commandLineArgs[i]);
    }
    
    // Fill remaining args with 0.0 if not provided
    for (int i = argc - 1; i < MAX_ARGS; i++) {
        commandLineArgs[i] = 0.0;
    }
}

// ###################### random argument bullshit

// Function to check if a string can be converted to a float or an integer
int isValidNumber(const char *str) {
    char *endptr;
    errno = 0; // Reset errno before strtof and strtol

    // Check for float
    float floatVal = strtof(str, &endptr);
    if (errno == 0 && *endptr == '\0') {
        return 1; // Valid float
    }

    // Reset errno for integer check
    errno = 0;

    // Check for integer
    long intVal = strtol(str, &endptr, 10);
    if (errno == 0 && *endptr == '\0' && intVal >= INT_MIN && intVal <= INT_MAX) {
        return 1; // Valid integer
    }

    return 0; // Not a valid float or integer
}

// ######### FUCK THIS MAN ########



   int main(int argc, char *argv[]) {
    // error checking, if no. of args is less than 2 
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename.ml>\n", argv[0]); // changed to fprintf to print to stderr instead of default data stream
        return 1;
    }

        // Check all argv inputs from argv[2] onward
    for (int i = 2; i < argc; i++) {
        if (!isValidNumber(argv[i])) {
            fprintf(stderr, "Error: Argument %d ('%s') is not a valid number.\n", i, argv[i]);
            return 1;
        }
    }

    //initalise buffer
    initBuffer();

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
    pCurrentTknIndex = 0;


    // debugging tokens
    printf("Tokens after tokenisation\n");
    for (int i = 0; i < TknIndex; i++) {
        print_token(Tokens[i]);
    }
    
    // Array to hold args starting from argv[2]
    char *args[MAX_ARGS];

    // Collect argv[2] onwards
    for (int i = 2; i < argc && argsCount < MAX_ARGS; i++) {
        args[argsCount++] = argv[i];
    }

    //consider arg0 type variables
    // Array to hold unique arguments
    int tokenCount = sizeof(Tokens) / sizeof(Tokens[0]);
    int uniqueCount = 0;

    UniqueArgs uniqueArgs;
    collectUniqueArgs(Tokens, tokenCount, &uniqueArgs);

    // check number of unique args and stuff matches
    if (uniqueArgs.count != argc -2) {
        fprintf(stderr, "! Error: Incorrect number of argument inputs for argument variables in file'\n");
        return 1;
    }
    
    

    // Parse the code and build the AST
    AstNode* result = pProgram(); 
    if (result != NULL) {

        // Convert the AST to C code
        toC(result);
        
        // Print the generated C code (for debugging)
        printf("Generated C code:\n%s\n", codeBuffer);

        conductAssiReplace(codeBuffer);

        // Write the generated C code to a file
        FILE *cFile = fopen("mlProgram.c", "w");
        if (cFile != NULL) {
            fputs(outputBuffer, cFile);
            fclose(cFile);
        } else {
            printf("Error writing C file.\n");
            return 1;
        }

        printf("Test passed!\n");
    } else {
        printf("Test failed!\n");
    }
    
    compileAndRunInC();
    cleanupAfterExec();
    
    // Free the buffer memory
    freeBuffer();

    return 0;
}
