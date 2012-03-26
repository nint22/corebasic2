/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: cbParse.h/c
 Desc: The lexical analyzer for a given cBasic program, as well
 as the compiler.
 
***************************************************************/

#ifndef __CBPARSE_H__
#define __CBPARSE_H__

#include "cbUtil.h"
#include "cbTypes.h"

/*** Main Lexical and Compiler Entry Points ***/

// Main parsing function; the root of all parsing events
void cbParse_ParseProgram(const char* Program, cbList* ErrorList);

// Main compiler function; the root of all compilation
void cbParse_CompileProgram(cbList* ErrorList);

/*** Parsing / Helper Functions ***/

// Error reporting associated with code compiling
void cbParse_RaiseError(cbList* ErrorList, cbError ErrorCode, size_t LineNumber);

// Parses a block of code, calling itself recursively for each new block, so that
// control code (i.e. for-loops and while-loops) do not intersect or collide
// Note that the StackDepth variable is to maintain correct block count
cbError cbParse_ParseBlock(char** Code, cbList* InstructionsList, cbList* DataList, cbList* VariablesList, cbList* JumpTable, cbList* LabelTable, int StackDepth);

// Parses a line of code, applying basic interpreter code
cbError cbParse_ParseLine(char* Line, cbList* InstructionsList, cbList* DataList, cbList* VariablesList, cbList* JumpTable);

// Given an expression (string of characters), parse using the Shunting-Yard algorithm:
// en.wikipedia.org/wiki/Shunting-yard_algorithm
// Returns an error state, or no error and posts a reverse polish notation of
// the expression as heap-allocated strings
cbError cbParse_ParseExpression(const char* Expression, cbList* OutputBuffer);

// Given a for loop function, pull out each expression and validate the structure of the for-loop function
// Note that this function internally allocates strings and will return a string that needs to be released explicitly
cbError cbParse_ParseFor(const char* Expression, char** IteratorExp, char** MinExp, char** MaxExp, char** IncrementExp);

// Returns a token from the string
const char* cbParse_GetToken(const char* String, size_t* TokenLength);

/*** Instruction-Parsing and Generation Wrappers ***/

// Save a given variable token into the variables list (if needed) and push the load variable command
cbError cbLang_LoadVariable(cbList* InstructionsList, cbList* VariablesList, char* Token, size_t TokenLength);

// Save a given literal (string, float, integer, boolean, etc.) into the data list (if needed) and push the load literal command
cbError cbLang_LoadLiteral(cbList* InstructionsList, cbList* DataList, char* Token, size_t TokenLength);

// Push the relative op into the instructions list
cbError cbLang_LoadOp(cbList* InstructionsList, char* Token, size_t TokenLength);

// End if inclusion guard
#endif
