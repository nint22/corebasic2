/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: cbCompile.h/c
 Desc: Compile the given lex-tree into the coreBasic byte-code.
 A lexical analysis tree is generated by using the parsing
 functions within "cbParse.h/c".
 
***************************************************************/

#ifndef __CBCOMPILE_H__
#define __CBCOMPILE_H__

#include "cbUtil.h"
#include "cbTypes.h"
#include "cbParse.h"

/*** Main Compiler Entry Points ***/

// Compile the given symbol table's lex tree into byte-code
bool cbParse_CompileProgram(cbSymbolsTable* SymbolsTable, cbList* ErrorList, cbVirtualMachine* Process);

/*** Internal Compilaton and Helper Functions ***/

// Build code by traversing the lex-tree left-right then inside (ops)
void cbParse_BuildNode(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList);

// Load a given instruction into the instructions list
// Returns the newly allocated instruction
cbInstruction* cbParse_LoadInstruction(cbSymbolsTable* SymbolsTable, cbOps Op, cbList* ErrorList, int Arg);

// Load a literal into the static memory segment and push a new loaddata call
void cbParse_LoadLiteral(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList);

// Builds instructions to load the variable at run-time onto the function stack
void cbParse_LoadVariable(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList);

// Load a jump instruction to the target label
void cbParse_LoadGoto(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList);

// Define a label that can be jumped to; duplicates raise errors
void cbParse_LoadLabel(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList);

// End if inclusion guard
#endif
