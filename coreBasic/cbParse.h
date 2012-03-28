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

// Define a macro that helps inturn define function pointers
// This is used heavily when doing production evaluation with the CFG
#define __cbParse_IsProduct(x) bool (x)(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)

/*** Main Lexical and Compiler Entry Points ***/

// Main parsing function; the root of all parsing events
void cbParse_ParseProgram(const char* Program, cbList* ErrorList);

// Main compiler function; the root of all compilation
//void cbParse_CompileProgram(cbList* ErrorList);

/*** Lexer / Parsing Functions ***/

// Parse a given line of text, up to the end of the buffer or newline
// Any erorrs are posted into the given error list
void cbParse_ParseLine(const char* Line, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if the given line (represented by tokens) is a statement
bool cbParse_IsStatement(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if the given line (represented by tokens) is a declaraton
bool cbParse_IsDeclaration(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if the given token is an ID
bool cbParse_IsID(const char* Token, size_t TokenLength);

// Returns true if it is either an integer, boolean, or float
bool cbParse_IsNumString(const char* Token, size_t TokenLength);

// Returns true if it is the start of a conditional block
bool cbParse_IsStatementIf(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Return true if it is the continuation of a conditional block
bool cbParse_IsStatementElif(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Return true if it is the last condition of a conditional block
bool cbParse_IsStatementElse(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Return true if it is the end of a block
bool cbParse_IsStatementEnd(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is the start of a while block
bool cbParse_IsStatementWhile(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is the start of a for block
bool cbParse_IsStatementFor(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it a goto statement
bool cbParse_IsStatementGoto(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is a label statement
bool cbParse_IsStatementLabel(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is a valid expression
bool cbParse_IsExpression(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is a valid expression list (an empty list is accepted)
bool cbParse_IsExpressionList(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is a valid term
bool cbParse_IsTerm(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is a valid unary
bool cbParse_IsUnary(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is a valid factor
bool cbParse_IsFactor(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is a valid bool (product from the formal cBasic CFG, NOT if it is a bool-string)
bool cbParse_IsBool(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is a valid join
bool cbParse_IsJoin(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns true if it is a given equality
bool cbParse_IsEquality(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns a token from the string
// Ignores all C-style double-slash comment structures until the newline
// Returns newlines as a single new-line token '\n' and returns a NULL at the end of the buffer
const char* cbParse_GetToken(const char* String, size_t* TokenLength);

/*** Helper Functions ***/

// Generic rule-applying function for binary rules (i.e. a -> {a + b | a - b | [optional c]}) with
// an optional "fall-through" rule. Given a list of tokens, and an array of operators to
// do the binary comparisons with, and the left and right symbols. Returns true if a
// valid recursive call of the rules
bool cbParse_IsBinaryProduction(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList, __cbParse_IsProduct(SymbolA), __cbParse_IsProduct(SymbolB), __cbParse_IsProduct(SymbolC), char** DelimList, size_t DelimCount);

// Error reporting associated with code compiling
void cbParse_RaiseError(cbList* ErrorList, cbError ErrorCode, size_t LineNumber);

// End if inclusion guard
#endif
