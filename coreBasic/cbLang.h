/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: cbLang.h/c
 Desc: The main interface to compile or load a coreBasic program.
 This manages the initial allocate and parsing preparation, as
 well as error handling and debugging to the end-developer.
 
***************************************************************/

#ifndef __CBLANG_H__
#define __CBLANG_H__

#include "cbUtil.h"
#include "cbTypes.h"
#include "cbParse.h"

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

#endif
