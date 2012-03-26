/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: cbUtil.h/c
 Desc: Generic helper functions and definitions used throughout
 the parser, compiler, and VM.
 
***************************************************************/

#include "cbUtil.h"

int cbLang_OpPrecedence(const char* String, size_t StringLength)
{
    // Error check
    if(String == NULL)
        return -1;
    
    // Single-char
    else if(StringLength == 1 && String[0] == '!')
        return 7;
    else if(StringLength == 1 && (String[0] == '*' || String[0] == '/' || String[0] == '%'))
        return 6;
    else if(StringLength == 1 && (String[0] == '+' || String[0] == '-'))
        return 5;
    else if(StringLength == 1 && (String[0] == '<' || String[0] == '>'))
        return 4;
    
    // Double-char
    else if(StringLength == 2 && (strncmp(String, ">=", StringLength) == 0 || strncmp(String, "<=", StringLength) == 0 ))
        return 4;
    else if(StringLength == 2 && (strncmp(String, "==", StringLength) == 0 || strncmp(String, "!=", StringLength) == 0 ))
        return 3;
    
    // And / or
    else if(StringLength == 3 && (strncmp(String, "and", StringLength) == 0))
        return 2;
    else if(StringLength == 2 && (strncmp(String, "or", StringLength) == 0))
        return 1;
    
    // Single-char
    else if(StringLength == 1 && (String[0] == '='))
        return 0;
    
    // Never validated
    return -1;
}

bool cbLang_OpLeftAssoc(const char* String, size_t StringLength)
{
    // Only '=' and '!' is right associated, so return true on everything except rank 0 and 
    if(StringLength == 1 && (String[0] == '=' || String[0] == '!'))
        return false;
    else
        return true;
}

void cbGetVersion(unsigned int* Major, unsigned int* Minor)
{
    *Major = __CBVERSION_MAJOR__;
    *Minor = __CBVERSION_MINOR__;
}

bool cbLang_IsInteger(const char* String, size_t StringLength)
{
    // Error check
    if(String == NULL)
        return false;
    
    for(size_t i = 0; i < StringLength; i++)
        if(!isdigit(String[i]))
            return false;
    return true;
}

bool cbLang_IsFunction(const char* String, size_t StringLength)
{
    // Error check
    if(String == NULL)
        return false;
    
    // Check against all first 19 ops (they are functions)
    for(int i = 0; i < cbOpsFuncCount; i++)
    {
        // Match?
        if(strlen(cbOpsNames[i]) == StringLength && strncmp(String, cbOpsNames[i], StringLength) == 0)
            return true;
    }
    
    // No match found
    return false;
}

bool cbLang_IsOp(const char* String, size_t StringLength)
{
    // Coalesced code; always a valid op if 0 or precedence rank
    return cbLang_OpLeftAssoc(String, StringLength) >= 0;
}

bool cbLang_IsFloat(const char* String, size_t StringLength)
{
    // Error check
    if(String == NULL)
        return false;
    
    // Find the decimal
    size_t DecimalIndex;
    for(DecimalIndex = 0; DecimalIndex <= StringLength; DecimalIndex++)
    {
        if(DecimalIndex == StringLength)
            return false; // Searched too far
        else if(String[DecimalIndex] == '.')
            break;
    }
    
    // The number before and after should be integers
    return cbLang_IsInteger(String, DecimalIndex) && cbLang_IsInteger(String + DecimalIndex + 1, StringLength - DecimalIndex - 1);
}

bool cbLang_IsString(const char* String, size_t StringLength)
{
    // Error check
    if(String == NULL)
        return false;
    
    // Must start and end with the quote character
    if(StringLength >= 2 && String[0] == '"' && String[StringLength - 1] == '"')
        return true;
    else
        return false;
}

bool cbLang_IsBoolean(const char* String, size_t StringLength)
{
    // Error check
    if(String == NULL)
        return false;
    
    if(StringLength == 4 && strncmp(String, "true", StringLength) == 0)
        return true;
    else if(StringLength == 5 && strncmp(String, "false", StringLength) == 0)
        return true;
    else
        return false;
}

bool cbLang_IsVariable(const char* String, size_t StringLength)
{
    // Must be at least 1 or more in length
    if(String == NULL || StringLength < 1)
        return false;
    
    // May only start with a regular alphabet char
    if(!isalpha(String[0]))
        return false;
    
    // The rest must be alpha-num
    for(size_t i = 1; i < StringLength; i++)
        if(!isalnum(String[i]))
            return false;
    
    // Is this a reserved keyword?
    if(cbLang_IsReserved(String, StringLength))
        return false;
    
    // Else, all good
    return true;
}

bool cbLang_IsReserved(const char* String, size_t StringLength)
{
    // Does this match with any of the reserved keywords
    for(int i = 0; i < cbOpsCount; i++)
    {
        size_t cbOpsLength = strlen(cbOpsNames[i]);
        if(cbOpsLength == StringLength && strncmp(String, cbOpsNames[i], StringLength) == 0)
            return true;
    }
    
    // Else, no match ever was found, so return false, not reserved
    return false;
}

bool cbList_CompareInt(void* A, void* B)
{
    // We know both should be cbVariables
    cbVariable* VarA = (cbVariable*)A;
    cbVariable* VarB = (cbVariable*)B;
    
    if(VarA->Type != VarB->Type)
        return false;
    else if(VarA->Type == cbType_Int && VarA->Data.Int != VarB->Data.Int)
        return false;
    // TODO: bool, string, float for comparison functions
    
    return true;
}

bool cbList_CompareString(void* A, void* B)
{
    // Both are strings, simple direct string compare is needed
    if(strcmp((const char*)A, (const char*)B) == 0)
        return true;
    else
        return false;
}

bool cbList_ComparePointer(void* A, void* B)
{
    // Straight address comparison
    return (A == B);
}

const char* const cbLang_GetErrorMsg(cbError ErrorCode)
{
    return cbErrorNames[ErrorCode];
}
