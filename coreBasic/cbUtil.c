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

bool cbLang_IsOp(const char* String, size_t StringLength)
{
    // Error check
    if(String == NULL)
        return false;
    
    // Single-char
    else if(StringLength == 1 && String[0] == '!')
        return true;
    else if(StringLength == 1 && (String[0] == '*' || String[0] == '/' || String[0] == '%'))
        return true;
    else if(StringLength == 1 && (String[0] == '+' || String[0] == '-'))
        return true;
    else if(StringLength == 1 && (String[0] == '<' || String[0] == '>'))
        return true;
    
    // Double-char
    else if(StringLength == 2 && (strncmp(String, ">=", StringLength) == 0 || strncmp(String, "<=", StringLength) == 0 ))
        return true;
    else if(StringLength == 2 && (strncmp(String, "==", StringLength) == 0 || strncmp(String, "!=", StringLength) == 0 ))
        return true;
    
    // And / or
    else if(StringLength == 3 && (strncmp(String, "and", StringLength) == 0))
        return true;
    else if(StringLength == 2 && (strncmp(String, "or", StringLength) == 0))
        return true;
    
    // Single-char
    else if(StringLength == 1 && (String[0] == '='))
        return true;
    
    // Never validated
    return false;
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

bool cbList_CompareLabelName(void* A, void *B)
{
    // We know that A is a cbLabel* object, while B is just a string
    return (strcmp(((cbLabel*)A)->LabelName, (char*)B) == 0);
}

void cbUtil_RaiseError(cbList* ErrorList, cbError ErrorCode, size_t LineNumber)
{
    // Allocate a new parse-error object
    cbParseError* NewError = malloc(sizeof(cbParseError));
    NewError->ErrorCode = ErrorCode;
    NewError->LineNumber = LineNumber;
    
    // Insert into list
    cbList_PushBack(ErrorList, NewError);
}

char* cbUtil_stralloc(const char* str)
{
    if(str == NULL)
        return NULL;
    
    return cbUtil_strnalloc(str, strlen(str));
}

char* cbUtil_strnalloc(const char* str, size_t strlength)
{
    if(str == NULL)
        return NULL;
    
    // String length used for size alloc and copy
    char* strcopy = malloc(strlength + 1);
    strncpy(strcopy, str, strlength);
    strcopy[strlength] = '\0';
    
    return strcopy;
}

bool cbUtil_IsComment(const char* str)
{
    // Ignore if null too early
    if(str == NULL || str[0] == '\0' || str[1] == '\0')
        return false;
    if(str[0] == '/' && str[1] == '/')
        return true;
    
    // Fall through: some other char-pair
    return false;
}

bool cbUtil_OpFromStr(const char* str, cbOps* OutOp)
{
    // Error check
    if(str == NULL)
        return false;
    
    // If single-char
    if(strlen(str) == 1)
    {
        switch(str[0])
        {
            case '=':   *OutOp = cbOps_Set;     break;
            case '!':   *OutOp = cbOps_Not;     break;
            case '*':   *OutOp = cbOps_Mul;     break;
            case '/':   *OutOp = cbOps_Div;     break;
            case '%':   *OutOp = cbOps_Mod;     break;
            case '+':   *OutOp = cbOps_Add;     break;
            case '-':   *OutOp = cbOps_Sub;     break;
            case '<':   *OutOp = cbOps_Less;    break;
            case '>':   *OutOp = cbOps_Greater; break;
            default: return false;
        }
        return true;
    }
    else if(strcmp(str, ">="))
    {
        *OutOp = cbOps_GreaterEq;
        return true;
    }
    else if(strcmp(str, "<="))
    {
        *OutOp = cbOps_LessEq;
        return true;
    }
    else if(strcmp(str, "=="))
    {
        *OutOp = cbOps_Eq;
        return true;
    }
    else if(strcmp(str, "!="))
    {
        *OutOp = cbOps_NotEq;
        return true;
    }
    else if(strcmp(str, "and"))
    {
        *OutOp = cbOps_And;
        return true;
    }
    else if(strcmp(str, "or"))
    {
        *OutOp = cbOps_Or;
        return true;
    }
    
    // Nothing found
    return false;
}

int g2Util_imin(int a, int b)
{
    return (a > b) ? b : a;
}

int g2Util_imax(int a, int b)
{
    return (a > b) ? a : b;
}
