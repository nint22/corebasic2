/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: cbUtil.h/c
 Desc: Defines all language operators, data types, and utility
 functions.
 
***************************************************************/

// Inclusion guard
#ifndef __CBUTIL_H__
#define __CBUTIL_H__

/*** Standard Includes ***/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "cbList.h"

#ifndef _WIN32
    #include <stdbool.h>
#endif

#include "cbTypes.h"

/*** Macros and Globals ***/

// Define the version string, major number, and minor number
#define __CBVERSION_MAJOR__ 0
#define __CBVERSION_MINOR__ 10

// Export macro definition
#ifdef _WIN32
    #define __cbEXPORT __declspec(dllexport)
#elif __linux__
    #define __cbEXPORT // Todo
#elif __APPLE__
    #define __cbEXPORT // Todo (done in framework rules)
#endif

// Posts the major and minor version
__cbEXPORT void cbGetVersion(unsigned int* Major, unsigned int* Minor);

/*** Operator Helpers ***/

// The higher the number, the more important it is (i.e. the higher priority it has)
int cbLang_OpPrecedence(const char* String, size_t StringLength);

// Returns true if left-to-right associative
bool cbLang_OpLeftAssoc(const char* String, size_t StringLength);

/*** Validation Functions ***/

// Returns true if the given string of given length is a number
bool cbLang_IsInteger(const char* String, size_t StringLength);

// Returns true if the given string of given length is a function (i.e. "disp")
bool cbLang_IsFunction(const char* String, size_t StringLength);

// Returns true if the given string of given length is an operand (i.e. "and")
bool cbLang_IsOp(const char* String, size_t StringLength);

// Returns true if float
bool cbLang_IsFloat(const char* String, size_t StringLength);

// Returns true if a string that starts and ends with quotes
bool cbLang_IsString(const char* String, size_t StringLength);

// Returns true if the given string is a boolean
bool cbLang_IsBoolean(const char* String, size_t StringLength);

// Returns true if the given string is a variable
bool cbLang_IsVariable(const char* String, size_t StringLength);

// Returns true if the given string is a reserved key-word
bool cbLang_IsReserved(const char* String, size_t StringLength);

/*** cbList Comparison Functions ***/

// Given two objects (integers), return true if they are the same
bool cbList_CompareInt(void* A, void* B);

// Given two pointers to strings, return true if they length-wise match (case sensitive)
bool cbList_CompareString(void* A, void* B);

// Given two pointers, return true if they point to the same address
bool cbList_ComparePointer(void* A, void* B);

/*** Error Helpers ***/

// Returns the english-language error string associated with the given error code
__cbEXPORT const char* const cbLang_GetErrorMsg(cbError ErrorCode);

/*** Macro-like Functions ***/

// Allocate, on the heap, a string that contains the given string buffer
// As with any object on the heap, it is up to the user to release it
inline char* cbUtil_stralloc(const char* str);

// Allocate, on the heap, a string that contains the given string buffer of given length
// As with any object on the heap, it is up to the user to release it
inline char* cbUtil_strnalloc(const char* str, size_t strlength);

// Min/max integer functions
inline int g2Util_imin(int a, int b);
inline int g2Util_imax(int a, int b);

// End of inclusion guard
#endif
