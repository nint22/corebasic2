/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: cbLang.h/c
 Desc: Defines all language operators, data types, and utility
 functions.
 
***************************************************************/

#ifndef __CBLANG_H__
#define __CBLANG_H__

#include "cbUtil.h"
#include "cbTypes.h"

/*** Init / Release Functions ***/

// Initialize a new virtual machine executing the given source code, within the given memory limitation, input and output streams, and screen size
// If there are any language, parsing, or formatting issues, a false is returned and a list of errors is posted. Else, true is returned
// Any and all errors are posted to the error list , a list of cbParseError objects which need to be released by the end-developer
__cbEXPORT bool cbInit_LoadSourceCode(cbVirtualMachine* Processor, unsigned long MemorySize, const char* Code, FILE* StreamOut, FILE* StreamIn, size_t ScreenWidth, size_t ScreenHeight, cbList* ErrorList);

// Initiaize a new virtual machine with the execisting byte code, within the given memory limitation, input and output streams, and screen size
__cbEXPORT cbError cbInit_LoadByteCode(cbVirtualMachine* Processor, unsigned long MemorySize, FILE* InFile, FILE* StreamOut, FILE* StreamIn, size_t ScreenWidth, size_t ScreenHeight);

// Write compiled code into the given file stream (c-style) in a format that
// can be loaded using "cbinit_LoadByteCode"
__cbEXPORT cbError cbInit_SaveByteCode(cbVirtualMachine* Processor, FILE* OutFile);

// Release a coreBasic simulator instance
__cbEXPORT cbError cbRelease(cbVirtualMachine* Processor);

/*** Syntax Highlighthing ***/

// Given a string, apply syntax coloring by returning a list of "cbHighlight_Type" elements.
// These elements define the range of the text and the type of the text in question. Note that
// the returned list must be released using cbList_Release(...)
__cbEXPORT cbList cbHighlightCode(const char* Code);

/*** Debugging Functions ***/

// Print the given simulation's instruction stack to the given file stream (commonly stdout)
__cbEXPORT void cbDebug_PrintInstructions(cbVirtualMachine* Processor, FILE* OutHandle);

// Print the given simulation's memory stack to the given file stream (commonly stdout)
__cbEXPORT void cbDebug_PrintMemory(cbVirtualMachine* Processor, FILE* OutHandle);

// Get the number of formal variables
__cbEXPORT size_t cbDebug_GetInstructionCount(cbVirtualMachine* Processor);

// Get the number of instructions
__cbEXPORT size_t cbDebug_GetVariableCount(cbVirtualMachine* Processor);

// Get the number of ticks from a process
__cbEXPORT size_t cbDebug_GetTicks(cbVirtualMachine* Processor);

// Get the active line we are executing
__cbEXPORT size_t cbDebug_GetLine(cbVirtualMachine* Processor);

// Get the formal operator name of a given instruction
__cbEXPORT const char* const cbDebug_GetOpName(cbOps Op);

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

#endif
