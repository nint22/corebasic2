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
#define __cbParse_IsProduct(x) cbLexNode* (x)(cbList* Tokens, size_t LineCount, cbList* ErrorList)

/*** Main Lexical and Compiler Entry Points ***/

// Main parsing function; the root of all parsing events
void cbParse_ParseProgram(const char* Program, cbList* ErrorList);

// Main compiler function; the root of all compilation
//void cbParse_CompileProgram(cbList* ErrorList);

/*** Lexer / Parsing Functions ***/

// Parse a given line of text, up to the end of the buffer or newline
// Any erorrs are posted into the given error list
cbLexNode* cbParse_ParseLine(const char* Line, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns a node if the given line (represented by tokens) is a statement
cbLexNode* cbParse_IsStatement(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList);

// Returns a node if the given line (represented by tokens) is a declaraton
cbLexNode* cbParse_IsDeclaration(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a true if the given token is an ID
bool cbParse_IsID(const char* Token, size_t TokenLength);

// Returns a true if it is either an integer, boolean, or float
bool cbParse_IsNumString(const char* Token, size_t TokenLength);

// Returns a node if it is the start of a conditional block
cbLexNode* cbParse_IsStatementIf(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is the continuation of a conditional block
cbLexNode* cbParse_IsStatementElif(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is the last condition of a conditional block
cbLexNode* cbParse_IsStatementElse(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is the end of a block
cbLexNode* cbParse_IsStatementEnd(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is the start of a while block
cbLexNode* cbParse_IsStatementWhile(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is the start of a for block
cbLexNode* cbParse_IsStatementFor(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it a goto statement
cbLexNode* cbParse_IsStatementGoto(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is a label statement
cbLexNode* cbParse_IsStatementLabel(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is a valid expression
cbLexNode* cbParse_IsExpression(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is a valid expression list (an empty list is accepted)
cbLexNode* cbParse_IsExpressionList(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is a valid term
cbLexNode* cbParse_IsTerm(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is a valid unary
cbLexNode* cbParse_IsUnary(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is a valid factor
cbLexNode* cbParse_IsFactor(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is a valid bool (product from the formal cBasic CFG, NOT if it is a bool-string)
cbLexNode* cbParse_IsBool(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is a valid join
cbLexNode* cbParse_IsJoin(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a node if it is a given equality
cbLexNode* cbParse_IsEquality(cbList* Tokens, size_t LineCount, cbList* ErrorList);

// Returns a token from the string
// Ignores all C-style double-slash comment structures until the newline
// Returns newlines as a single new-line token '\n' and returns a NULL at the end of the buffer
const char* cbParse_GetToken(const char* String, size_t* TokenLength);

/*** CFG Helper Functions ***/

// Generic rule-applying function for a single-keyword form (i.e. a->{[keyword]})
cbLexNode* cbParse_IsKeywordProduction(cbList* Tokens, size_t LineCount, cbList* ErrorList,  const char* Keyword, cbSymbol Symbol);

// Generic rule-applying function for the single-keyword form with a boolean expression
// (i.e. a -> {[keyword](bool)}
cbLexNode* cbParse_IsKeywordBoolProduction(cbList* Tokens, size_t LineCount, cbList* ErrorList, const char* Keyword, cbSymbol Symbol);

// Generic rule-applying function for binary rules (i.e. a -> {a + b | a - b | [optional c]}) with
// an optional "fall-through" rule. Given a list of tokens, and an array of operators to
// do the binary comparisons with, and the left and right symbols. Returns a node if a
// valid recursive call of the rules
cbLexNode* cbParse_IsBinaryProduction(cbList* Tokens, size_t LineCount, cbList* ErrorList, __cbParse_IsProduct(SymbolA), __cbParse_IsProduct(SymbolB), __cbParse_IsProduct(SymbolC), char** DelimList, size_t DelimCount);

// Error reporting associated with code compiling
void cbParse_RaiseError(cbList* ErrorList, cbError ErrorCode, size_t LineNumber);

/*** Lexical Tree Functions ***/

// Allocate a node with with all pointers to NULL, and the node is either a symbol or terminal
cbLexNode* cbLex_CreateNode(cbLexNodeType Type);

// Allocate a symbol node with the given symbol type
cbLexNode* cbLex_CreateNodeSymbol(cbSymbol Symbol);

// Allocate a terminal with either a literal (int, float, bool, string), variable, or operator
cbLexNode* cbLex_CreateNodeI(int Integer);
cbLexNode* cbLex_CreateNodeF(float Float);
cbLexNode* cbLex_CreateNodeB(bool Boolean);
cbLexNode* cbLex_CreateNodeS(const char* StringLiteral);
cbLexNode* cbLex_CreateNodeV(const char* VariableName);
cbLexNode* cbLex_CreateNodeO(cbOps Op);

// Remove the given node (and all children nodes) as well as release all data
// Note that if the data contains a string, that too is released from the heap
// Note: sets the given pointer to NULL
void cbLex_DeleteNode(cbLexNode** Node);

// End if inclusion guard
#endif
