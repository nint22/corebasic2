/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: cbTypes.h
 Desc: Defines all internal and external data types and structures.
 
***************************************************************/

// Inclusion guard
#ifndef __CBTYPES_H__
#define __CBTYPES_H__

#include "cbUtil.h"

/*** Machine / Simulation Definition ***/

// Interrupt types (only three actual interrupts, one "none")
static const int cbInterruptCount = 4;
typedef enum __cbInterrupt
{
    cbInterrupt_None,       // No interruption
    cbInterrupt_Pause,      // Wait for specific "enter" key, read it off
    cbInterrupt_GetKey,     // Wait on any key, read it off, push onto stack
    cbInterrupt_Input,      // Wait for specific "enter" key, push all read onto stack
} cbInterrupt;

// The processor / interpreter state
typedef struct __cbVirtualMachine
{
    // Current states / memory positions (Similar to registers)
    size_t StackPointer;          // The top of the stack (top of the active function frame)
    size_t StackBasePointer;      // Current base address of the active function frame
    size_t HeapPointer;           // The start of the heap
    size_t DataPointer;           // Which instruction are we on
    size_t DataVarCount;          // How many variables there are
    size_t InstructionPointer;    // Which instruction are we on
    size_t Ticks;                 // Total number of ticks in the process
    
    /*
     Memory layout is as follows:
     Stack: Grows to a lower address (so the base pointer + var offsets == valid address)
     Static data: Any sort of constants / initial values
     Code: Instructions (Ops, with single args)
     
     HIGH ADDRESS
     
     +--------+
     |        | <-- Stack base pointer
     | Stack  |     Note: Points to the current frame's variables (below stack pointer)
     |   |    |
     |   V    |
     |--------| <-- Stack pointer (Changes over time)
     |        |     Note: Points to the base address of the current function frame
     ....
     |        |
     |--------| <-- Heap start
     |        |
     | Static | <-- Note: "DataVarCount" shows how many true
     |  Data  |     variables there are, since some data might be arrays
     |        |
     |--------| <-- Data pointer (constant)
     |        |
     |  Code  | <-- Instruction pointer (Changes over time)
     |        |     Note: Code contains data as well (as args)
     +--------+
     
     LOW ADDRESS
     
     Graphical memory is allocated as its own seperate chunk:
     Total byte count: (width * height), 1 byte per pixel
     Note that color is gray-scale and only ranges from [0, 3]
     
     +--------+
     |        |
     | Screen | <-- Origin is bottom-left, 1 byte per pixel
     |        |
     +--------+
     
     */
    
    // Stack memory [Array of unsigned bytes]
    // This is unique per each implementation's need
    size_t MemorySize;
    void* Memory;
    
    // Simulation has been halted / stopped
    bool Halted;
    
    // Interrupt state, waiting for user input event
    cbInterrupt InterruptState;
    
    // I/O buffers (just c-style file streams; i.e. just like stdin)
    FILE* StreamOut;
    FILE* StreamIn;
    
    // Screen dimensions
    // Note that the screen origin in the bottom-left
    size_t ScreenWidth, ScreenHeight;
    unsigned char* ScreenBuffer;
    
    // The current line we are executing in a simulation
    size_t LineIndex;
    
} cbVirtualMachine;

// Operator set
static const int cbOpsCount = 37;
static const int cbOpsFuncCount = 17;
typedef enum __cbOps
{
    // Program control
    cbOps_If,
    cbOps_Elif,
    cbOps_Else,
    cbOps_For,
    cbOps_While,
    cbOps_End,
    cbOps_Pause,
    cbOps_Label,
    cbOps_Goto,
    cbOps_Exec,
    cbOps_Return,
    cbOps_Halt,
    
    // Program I/O
    cbOps_Input,
    cbOps_Disp,
    cbOps_Output,
    cbOps_GetKey,
    cbOps_Clear,
    
    // Misc.
    cbOps_Func,
    cbOps_Set,
    
    // Primitive commands (i.e. algebra) (executes the last two on the stack)
    cbOps_Add,
    cbOps_Sub,
    cbOps_Mul,
    cbOps_Div,
    cbOps_Mod,
    
    // Boolean algebra
    cbOps_Eq,
    cbOps_NotEq,
    cbOps_Greater,
    cbOps_GreaterEq,
    cbOps_Less,
    cbOps_LessEq,
    cbOps_Not,
    cbOps_And,
    cbOps_Or,
    
    // Hidden internal ops
    cbOps_LoadData,  // Push a literal from the data section (arg is the offset from the data base)
    cbOps_LoadVar,   // Push the variable's offset from the stack base into the stack (arg is the offset from stack base)
    cbOps_AddStack,  // Add the number of bytes (positive or negative) to the stack pointer by arg bytes
    cbOps_Nop,       // No-Operator; commonly used to store meta information (i.e. debugging symbols) in the arg int
} cbOps;

// Instructions are a combination of an op and a data offset (if needed)
typedef struct __cbInstruction
{
    cbOps Op;       // Instruction we are to execute
    int Arg;        // Location, relative or global, as the arg (optional)
} cbInstruction;

// English-language op names (keywords)
static const char cbOpsNames[cbOpsCount][16] =
{
    "if",
    "elif",
    "else",
    "for",
    "while",
    "end",
    "pause",
    "label",
    "goto",
    "exec",
    "return",
    "halt",
    "input",
    "disp",
    "output",
    "getKey",
    "clear",
    "func",
    "=",
    "+",
    "-",
    "*",
    "/",
    "%",
    "==",
    "!=",
    ">",
    ">=",
    "<",
    "<=",
    "!",
    "and",
    "or",
    
    // Private:
    "loaddata",
    "loadvar",
    "addstack",
    "nop",
};

/*** Context-Free Grammar Language Definition ***/

// Based on the CFG formal language defintion found
// here: code.google.com/p/corebasic/wiki/cBasic
// Basic rule is: Symbol -> string of terminals

// Note that the production rules are implemented as functions within cbLang

// Symbols list
static const int cbSymbolCount = 28;
typedef enum __cbSymbol
{
    // No symbol / undef
    cbSymbol_None,
    
    // Character and string type definitions
    cbSymbol_NumChar,
    cbSymbol_AlphaChar,
    cbSymbol_AlphaNumChar,
    cbSymbol_AlphaString,
    cbSymbol_NumString,
    cbSymbol_AlphaNumString,
    cbSymbol_ID,
    
    // Basic Structures
    // All production rules should derive from here
    cbSymbol_Line,
    cbSymbol_Statements,
    cbSymbol_Statement,
    cbSymbol_Declaration,
    
    // Statements
    cbSymbol_StatementIf,
    cbSymbol_StatementElif,
    cbSymbol_StatementElse,
    cbSymbol_StatementWhile,
    cbSymbol_StatementFor,
    cbSymbol_StatementGoto,
    cbSymbol_StatementLabel,
    
    // Expressions
    cbSymbol_Bool,
    cbSymbol_Join,
    cbSymbol_Equality,
    cbSymbol_Expression,
    cbSymbol_ExpressionList,
    cbSymbol_Term,
    cbSymbol_Unary,
    cbSymbol_Factor,
    
    // Misc.
    cbSymbol_End,
} cbSymbol;

/*** Variable Type Definition ***/

// Data types
typedef enum __cbVariableType
{
    cbVariableType_Int,
    cbVariableType_Float,
    cbVariableType_Bool,
    cbVariableType_String,
    cbVariableType_Offset, // Equivalent to a pointer (internal use only)
} cbVariableType;

// Variable holders
typedef struct __cbVariable
{
    cbVariableType Type;
    union
    {
        int Int;
        float Float;
        bool Bool;
        char* String;
        int Offset;
    } Data;
} cbVariable;


/*** Lexical / Symbol-Products Tree ***/

// Define the types of lexical-analysis nodes in a lex-tree
typedef enum __cbLexNodeType
{
    cbLexNodeType_Symbol,   // Defines a Symbol -> Product relationship
    cbLexNodeType_Terminal, // Define a Product -> Terminal relationship
} cbLexNodeType;

typedef enum __cbLexIDType
{
    cbLexIDType_Int,
    cbLexIDType_Float,
    cbLexIDType_Bool,
    cbLexIDType_StringLit,
    cbLexIDType_Variable,   // Still uses the same string buffer
    cbLexIDType_Op,
} cbLexIDType;


// Lexical analysis token ID
typedef struct __cbLexID
{
    // Note that "cbType_Offset" represents a variable name (stored in string)
    cbLexIDType Type;
    union
    {
        int Integer;
        float Float;
        bool Boolean;
        char* String;
        cbOps Op;
    } Data;
} cbLexID;

// Define
typedef struct __cbLexNode
{
    // What kind of lex-node is this?
    cbLexNodeType Type;
    
    // Lex content
    union // Anonymous
    {
        cbSymbol Symbol;        // Symbol type (Expression, Statement, etc..)
        cbLexID Terminal;       // Operators, literals, and variables (123, "abc", True, etc..)
    } Data;
    
    // Left and right nodes in our binary tree
    struct __cbLexNode* Left;
    struct __cbLexNode* Middle; // Only used with ops
    struct __cbLexNode* Right;
    
    // Line number this lex item was taken from
    size_t LineNumber;
    
} cbLexNode;

// Internal symbols table for parsing, lex-tree building, etc.
typedef struct __cbSymbolsTable
{
    // Number of block stacks (loops and conditionals)
    size_t BlockDepth;
    
    // A root node for each line of code
    cbList LexTree;
    
    // Pseudo op-codes used during the compiling process
    // Though the ops are correct, many of the arguments
    // are not yet mapped to memory
    cbList InstructionsList;    // List of all instructions (cbInstruction)
    cbList DataList;            // List of all literals (cbVariable)
    cbList VariablesList;       // List of variable offsets (c-style strings)
    cbList JumpTable;           // Table of jump instructions (mixed array of 0: jump operator (ref, not new), then 1: label-name (new obj, and new internal string))
    cbList LabelTable;          // Table of jump destinations
    cbList BlockStack;          // Active stack of blocks during program compilation (references are not unique, should never delete)
    
} cbSymbolsTable;

/*** Compiler Structs ***/

// A helper data structure to track all labels and their addresses,
// used in the jump table at the end of block processing
typedef struct __cbLabel
{
    size_t Index;
    char* LabelName;
} cbLabel;

// A helper data structure to track all traget jump locations
// This is commonly used when needing to maintain control-loop
// header locations
typedef struct __cbJumpTarget
{
    cbSymbol Symbol;            // The symbol this block is associated with (while, for, if, etc..)
    size_t Index;               // Target address index (from the front of the instructions list)
    cbInstruction* Instruction; // The jump instruction
} cbJumpTarget;

/*** Error Reporting ***/

// Failure reasons
// Note that these are both parsing, compiling,
// and run-time error definitions
static const int cbErrorCount = 18;
typedef enum __cbError
{
    cbError_None,
    cbError_Null,
    cbError_Overflow,
    cbError_UnknownOp,
    cbError_UnknownToken,
    cbError_UnknownLine,
    cbError_DivZero,
    cbError_TypeMismatch,
    cbError_ParenthMismatch,
    cbError_Halted,
    cbError_Assignment,
    cbError_BlockMismatch,
    cbError_ParseInt,
    cbError_ParseFloat,
    cbError_MissingArgs,
    cbError_MissingLabel,
    cbError_InvalidID,
    cbError_ConstSet,
} cbError;

// English-language error names
static const char cbErrorNames[cbErrorCount][64] =
{
    "No Error",
    "Null variable",
    "Overflow or out of bounds access",
    "Unknown operator",
    "Unknown token",
    "Unknown line formation",
    "Tried to divide by zero",
    "Type mismatch",
    "Parentheses mismatch",
    "Process halted",
    "Cannot assign expression",
    "Block mismatch",
    "Failed to parse integer",
    "Failed to parse float",
    "Missing arguments",
    "Missing label",
    "Invalid variable name",
    "Assigning a constant",
};

// Define a parsing error which is an error code and a line number
typedef struct __cbParseError
{
    // Line number
    size_t LineNumber;
    
    // Error ID
    cbError ErrorCode;
    
} cbParseError;

/*** Syntax Highlighting ***/

// Define all possible token types
static const int cbTokenTypeCount = 6;
typedef enum __cbTokenType
{
    cbTokenType_Comment,
    cbTokenType_Variable,
    cbTokenType_Function,
    cbTokenType_StringLit,
    cbTokenType_NumericalLit,
    cbTokenType_Keyword,
} cbTokenType;

// A type representing the range of text and the word type that is associated with it
typedef struct __cbHighlightToken
{
    size_t Start, Length;
    cbTokenType TokenType;
} cbHighlightToken;

// End of inclusion guard
#endif
