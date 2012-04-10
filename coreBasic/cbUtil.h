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

/*** Validation Functions ***/

// Returns true if the given string of given length is a number
bool cbLang_IsInteger(const char* String, size_t StringLength);

// Returns true if the given string of given length is an operand (i.e. "and")
bool cbLang_IsOp(const char* String, size_t StringLength);

// Returns true if float
bool cbLang_IsFloat(const char* String, size_t StringLength);

// Returns true if a string that starts and ends with quotes
bool cbLang_IsString(const char* String, size_t StringLength);

// Returns true if the given string is a boolean
bool cbLang_IsBoolean(const char* String, size_t StringLength);

// Returns true if the given string is a reserved key-word
bool cbLang_IsReserved(const char* String, size_t StringLength);

/*** cbList Comparison Functions ***/

// Given two pointers to strings, return true if they length-wise match (case sensitive)
bool cbList_CompareString(void* A, void* B);

// Given two pointers, return true if they point to the same address
bool cbList_ComparePointer(void* A, void* B);

// Given two pointers, the first being of a cbLabel* and the second being a c-string (char*), return
// true if they match through strcmp (case sensitive)
bool cbList_CompareLabelName(void* A, void *B);

/*** Macro-like Functions ***/

// Error reporting associated with code compiling
inline void cbUtil_RaiseError(cbList* ErrorList, cbError ErrorCode, size_t LineNumber);

// Allocate, on the heap, a string that contains the given string buffer
// As with any object on the heap, it is up to the user to release it
inline char* cbUtil_stralloc(const char* str);

// Allocate, on the heap, a string that contains the given string buffer of given length
// As with any object on the heap, it is up to the user to release it
inline char* cbUtil_strnalloc(const char* str, size_t strlength);

// Returns true if the given string starts with a double-slash c-style comment
inline bool cbUtil_IsComment(const char* str);

// Returns the op associated with the given string, or Op_None if not found
bool cbUtil_OpFromStr(const char* str, cbOps* OutOp);

// Min/max integer functions
inline int g2Util_imin(int a, int b);
inline int g2Util_imax(int a, int b);

// End of inclusion guard
#endif
