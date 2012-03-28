/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#include "cbParse.h"

void cbParse_ParseProgram(const char* Program, cbList* ErrorList)
{
    // Create our symbols table (just for lexical analysis help for now)
    cbSymbolsTable SymbolsTable;
    cbList_Init(&SymbolsTable.LocalStack);
    
    // Keep an active token pointer; this changes over time
    const char* ActiveLine = Program;
    
    // The line we are on
    size_t LineCount = 1;
    
    // Keep parsing until done
    while(true)
    {
        // Parse this line
        cbParse_ParseLine(ActiveLine, &SymbolsTable, LineCount, ErrorList);
        
        // Get the next new-line
        ActiveLine = strchr(ActiveLine, '\n');
        
        // If this is the end of the program, stop parsing
        if(ActiveLine == NULL || ActiveLine[1] == '\0')
            break;
        else
        {
            // Point to the next line, and grow the line count
            ActiveLine++;
            LineCount++;
        }
    }
    
    // If the local stack is not empty, then there is a dangling end-block
    if(cbList_GetCount(&SymbolsTable.LocalStack) > 0)
        cbParse_RaiseError(ErrorList, cbError_BlockMismatch, LineCount);
    
    // Done parsing
}

void cbParse_ParseLine(const char* Line, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Keep an active token pointer; this changes over time
    const char* ActiveToken = Line;
    size_t TokenLength = 0;
    
    // Build a list of all tokens, from left to right
    // Each token is a heap-allocated string
    cbList Tokens;
    cbList_Init(&Tokens);
    
    // Keep parsing until done
    while(true)
    {
        // Get the active token (previous address + previous length)
        ActiveToken = cbParse_GetToken(ActiveToken + TokenLength, &TokenLength);
        
        // If this is the end of the stream, or the end of the line, break out
        if(ActiveToken == NULL || (TokenLength <= 1 && ActiveToken[0] == '\n'))
            break;
        
        // Add token into the list
        cbList_PushBack(&Tokens, cbUtil_strnalloc(ActiveToken, TokenLength));
    }
    
    // Debugging
    printf("Line %lu: (%lu tokens)\n\t", LineCount, cbList_GetCount(&Tokens));
    for(int i = 0; i < cbList_GetCount(&Tokens); i++)
        printf("'%s', ", cbList_GetElement(&Tokens, i));
    printf("\n");
    // Debugging
    
    // Only process if there are tokens
    if(cbList_GetCount(&Tokens) > 0)
    {
        // Line production rule:
        // Line -> {Statement | Declaration}, but if we have an active conditional stack, validate elif, else, and end product ruels
        if(!cbParse_IsDeclaration(&Tokens, SymbolsTable, LineCount, ErrorList) && !cbParse_IsStatement(&Tokens, SymbolsTable, LineCount, ErrorList))
            cbParse_RaiseError(ErrorList, cbError_UnknownLine, LineCount);
        
        // Release the tokens
        while(cbList_GetCount(&Tokens) > 0)
            free(cbList_PopFront(&Tokens));
    }
}

bool cbParse_IsStatement(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Statement production rule:
    // Statement -> {StatementIf? | StatementWhile? | StatementFor? | StatementGoto? | StatementLabel? | Expression}
    // Note: Order is important, because if we don't check for if and while before checking for functions, the
    // expression product will sudgest that a "while(something)" looks like a function call
    
    // Special rule: if the local stack is non-zero (i.e. we are within a coditional block), look for
    // both If, Elif, Else, and End statements
    size_t LocalStackDepth = cbList_GetCount(&SymbolsTable->LocalStack);
    
    // Apply all production rules...
    if(cbParse_IsStatementIf(Tokens, SymbolsTable, LineCount, ErrorList))
        return true;
    else if(LocalStackDepth > 0 && cbParse_IsStatementElif(Tokens, SymbolsTable, LineCount, ErrorList))
        return true;
    else if(LocalStackDepth > 0 && cbParse_IsStatementElse(Tokens, SymbolsTable, LineCount, ErrorList))
        return true;
    else if(LocalStackDepth > 0 && cbParse_IsStatementEnd(Tokens, SymbolsTable, LineCount, ErrorList))
        return true;
    else if(cbParse_IsStatementWhile(Tokens, SymbolsTable, LineCount, ErrorList))
        return true;
    else if(cbParse_IsStatementFor(Tokens, SymbolsTable, LineCount, ErrorList))
        return true;
    else if(cbParse_IsStatementGoto(Tokens, SymbolsTable, LineCount, ErrorList))
        return true;
    else if(cbParse_IsStatementLabel(Tokens, SymbolsTable, LineCount, ErrorList))
        return true;
    else if(cbParse_IsExpression(Tokens, SymbolsTable, LineCount, ErrorList))
        return true;
    
    // No statement match found, fail
    return false;
}

bool cbParse_IsDeclaration(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Declaration production rule:
    // Declaration -> {ID = Expression}
    
    // Must have a minimum of three or more tokens
    if(cbList_GetCount(Tokens) < 3)
        return false;
    
    // First token must always be an ID
    char* DestID = cbList_GetElement(Tokens, 0);
    if(!cbParse_IsID(DestID, strlen(DestID)))
        return false;
    
    // Second token must always be the equal op
    char* AssignmentOp = cbList_GetElement(Tokens, 1);
    if(strlen(AssignmentOp) != 1 || AssignmentOp[0] != '=')
        return false;
    
    // The rest is assumed an expression
    cbList ExpressionTokens;
    cbList_Subset(Tokens, &ExpressionTokens, 2, cbList_GetCount(Tokens) - 2);
    
    // If is expression, valid
    return cbParse_IsExpression(&ExpressionTokens, SymbolsTable, LineCount, ErrorList);
}

bool cbParse_IsID(const char* Token, size_t TokenLength)
{
    // Note: the production rule here is complex, but can be simplified
    // to non-recursive rules
        
    // Must be at least 1 char long
    if(Token == NULL || TokenLength < 1)
        return false;
    
    // Just char must be just alpha
    if(!isalpha(Token[0]))
        return false;
    
    // The rest must be alpha-num
    for(size_t i = 1; i < TokenLength; i++)
        if(!isalnum(Token[0]))
            return false;
    
    // Otherwise, all good!
    return true;
}

bool cbParse_IsNumString(const char* Token, size_t TokenLength)
{
    // Note: the production rule here is complex, but can be simplified
    // to non-recursive rules
    
    // Must be at least 1 char long
    if(Token == NULL || TokenLength < 1)
        return false;
    
    // If true or false, return true
    if(strncmp(Token, "true", 5) == 0 || strncmp(Token, "false", 6) == 0)
        return true;
    
    // Could be a whole integer or float
    bool IsInteger = true;
    int DotCount = 0;
    for(size_t i = 0; i < TokenLength; i++)
    {
        if(Token[i] == '.')
            DotCount++;
        else if(!isnumber(Token[i]))
            IsInteger = false;
    }
    
    // Either integer or float
    if(IsInteger && DotCount <= 1)
        return true;
    
    // Otherwise, failed
    return false;
}

bool cbParse_IsStatementIf(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // If production rule:
    // StatementIf -> {if(Bool) Lines end | if(Bool) Lines StatementElif? | if(Bool) Lines StatementElse?}
    
    // If conditions always start with if, (, <bool>, ) ..
    if(cbList_GetCount(Tokens) < 4)
        return false;
    
    // First token must be if
    char* IfToken = cbList_GetElement(Tokens, 0);
    if(strcmp(IfToken, "if") != 0)
        return false;
    
    // Second and last token should always be '(' and ')' respectivly
    char* FirstParenth = cbList_GetElement(Tokens, 1);
    char* LastParenth = cbList_PeekBack(Tokens);
    if(strcmp(FirstParenth, "(") != 0 || strcmp(LastParenth, ")") != 0)
        return false;
    
    // Pase the boolean expression within the parenth
    cbList BoolSubset;
    cbList_Subset(Tokens, &BoolSubset, 2, cbList_GetCount(Tokens) - 3);
    
    // Boolean expression must be valid
    if(!cbParse_IsBool(&BoolSubset, SymbolsTable, LineCount, ErrorList))
        return false;
    
    // We can continue parsing lines, but we must remember that we are executing other statements until we hit
    cbList_PushBack(&SymbolsTable->LocalStack, NULL); // For now, just a placeholder
    
    // Good to go
    return true;
}

bool cbParse_IsStatementElif(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Elif production rule:
    // StatementElif -> {elif(Bool) Lines end | elif(Bool) Lines StatementElif? | elif(Bool) Lines StatementElse?}
    
    // Only true if there is at least something to pop off the local conditional stack
    if(cbList_GetCount(&SymbolsTable->LocalStack) <= 0)
        return false;
    
    // If conditions always start with elif, (, <bool>, ) ..
    if(cbList_GetCount(Tokens) < 4)
        return false;
    
    // First token must be if
    char* IfToken = cbList_GetElement(Tokens, 0);
    if(strcmp(IfToken, "elif") != 0)
        return false;
    
    // Second and last token should always be '(' and ')' respectivly
    char* FirstParenth = cbList_GetElement(Tokens, 1);
    char* LastParenth = cbList_PeekBack(Tokens);
    if(strcmp(FirstParenth, "(") != 0 || strcmp(LastParenth, ")") != 0)
        return false;
    
    // Pase the boolean expression within the parenth
    cbList BoolSubset;
    cbList_Subset(Tokens, &BoolSubset, 2, cbList_GetCount(Tokens) - 3);
    
    // Boolean expression must be valid
    return cbParse_IsBool(&BoolSubset, SymbolsTable, LineCount, ErrorList);
}

bool cbParse_IsStatementElse(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Else production rule:
    // StatementElse -> {else Lines end}
    
    // Only true if there is at least something to pop off the local conditional stack
    if(cbList_GetCount(&SymbolsTable->LocalStack) <= 0)
        return false;
    
    // Only true if one single token: "else"
    if(cbList_GetCount(Tokens) != 1)
        return false;
    
    // Get the token; only a true else if string match
    char* Token = cbList_PeekFront(Tokens);
    return (strcmp(Token, "else") == 0);
}

bool cbParse_IsStatementEnd(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Only true if there is at least something to pop off the local conditional stack
    if(cbList_GetCount(&SymbolsTable->LocalStack) <= 0)
        return false;
    
    // Only true if one single token: "end"
    if(cbList_GetCount(Tokens) != 1)
        return false;
    
    // Get the token and pop off a unit from the local stack
    char* Token = cbList_PeekFront(Tokens);
    if(strcmp(Token, "end") == 0)
    {
        cbList_PopBack(&SymbolsTable->LocalStack);
        return true;
    }
    
    // Else, not an end line
    return false;
}

bool cbParse_IsStatementWhile(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // While production rule:
    // StatementWhile -> {while(Bool) Lines end}
    
    // If it is at least four tokens: "while", parent-pair, and the bool-arg
    size_t TokenCount = cbList_GetCount(Tokens);
    if(TokenCount >= 4)
    {
        // Make sure it is "while" and parenth group
        char* ID = cbList_PeekFront(Tokens);
        char* StartParenth = cbList_GetElement(Tokens, 1);
        char* EndParenth = cbList_PeekBack(Tokens);
        if(strcmp(ID, "while") == 0 && strcmp(StartParenth, "(") == 0 && strcmp(EndParenth, ")") == 0)
        {
            // Make sure that the subset is a boolean expression
            cbList ExpressionList;
            cbList_Subset(Tokens, &ExpressionList, 2, TokenCount - 3);
            if(cbParse_IsBool(&ExpressionList, SymbolsTable, LineCount, ErrorList))
            {
                // Valid, and we are starting a new local stack
                cbList_PushBack(&SymbolsTable->LocalStack, NULL);
                return true;
            }
        }
    }
    
    // Failed
    return false;
}

bool cbParse_IsStatementFor(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // For production rule:
    // StatementFor -> {for(ID, Expression, Expression, Expression) Lines end}
    
    // If it is at least 10 tokens: "for", parenth-pair, comma delimiters, and each arg
    size_t TokenCount = cbList_GetCount(Tokens);
    if(TokenCount >= 10)
    {
        // Make sure it is "for" and parenth group
        char* ID = cbList_PeekFront(Tokens);
        char* StartParenth = cbList_GetElement(Tokens, 1);
        char* EndParenth = cbList_PeekBack(Tokens);
        if(strcmp(ID, "for") == 0 && strcmp(StartParenth, "(") == 0 && strcmp(EndParenth, ")") == 0)
        {
            // The first param must be an ID followed by a comma
            char* LoopID = cbList_GetElement(Tokens, 2);
            char* CommaID = cbList_GetElement(Tokens, 3);
            if(cbParse_IsID(LoopID, strlen(LoopID)) && strcmp(CommaID, ",") == 0)
            {
                // Create a subset of the tokens left
                cbList Subset;
                cbList_Subset(Tokens, &Subset, 4, TokenCount - 5);
                size_t SubsetCount = cbList_GetCount(&Subset);
                
                // If this is a valid expressions list and it is only three expressions...
                size_t ExpressionCount;
                // We NEED to know how many sub-args are at this point
                // Thus, we need to have the actual parse tree built at this point
                return false;
            }
        }
    }
    
    // Failed
    return false;
}

bool cbParse_IsStatementGoto(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Goto statements should always be two tokens
    if(cbList_GetCount(Tokens) != 2)
        return false;
    
    // First must always match "goto"
    if(strcmp(cbList_GetElement(Tokens, 0), "goto") != 0)
        return false;
    
    // Second must always be an ID
    char* ID = cbList_GetElement(Tokens, 1);
    if(!cbParse_IsID(ID, strlen(ID)))
        return false;
    
    // Else, all good
    return true;
}

bool cbParse_IsStatementLabel(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Goto statements should always be three tokens
    if(cbList_GetCount(Tokens) != 3)
        return false;
    
    // First must always match "label"
    if(strcmp(cbList_GetElement(Tokens, 0), "label") != 0)
        return false;
    
    // Second must always be an ID
    char* ID = cbList_GetElement(Tokens, 1);
    if(!cbParse_IsID(ID, strlen(ID)))
        return false;
    
    // Final is just a single colon
    char* Colon = cbList_GetElement(Tokens, 2);
    if(strlen(Colon) != 1 || Colon[0] != ':')
        return false;
    
    // Else, all good
    return true;
}

bool cbParse_IsExpression(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Expressions production are the largest, but not complex, group
    // Expression prodction rules:
    /*
     Expression -> {Expression + Term | Expression - Term | ID(ExpressionList) | Term}
     ExpressionList -> {ExpressionList, Expression | Expression | Empty}
     Term -> {Term * Unary | Term / Unary | Term % Unary | Unary}
     Unary -> {!Unary | -Unary | Factor}
     Factor -> {(bool) | ID | 'true' | 'false' | IntString? | Float | String Literal}
     Bool -> {Bool or Join | Join}
     Join -> {Join and Equality | Equality}
     Equality -> {Expression == Expression | Expression != Expression | Expression < Expression | Expression <= Expression | Expression > Expression | Expression >= Expression}
    */
    
    // Function-call validation:
    // If it is at least four operators, check if we can apply the function product "ID(ExpressionList)"
    size_t TokenCount = cbList_GetCount(Tokens);
    if(TokenCount >= 4)
    {
        // Make sure it is an ID and parenth group
        char* ID = cbList_PeekFront(Tokens);
        char* StartParenth = cbList_GetElement(Tokens, 1);
        char* EndParenth = cbList_PeekBack(Tokens);
        if(cbParse_IsID(ID, strlen(ID)) && strcmp(StartParenth, "(") == 0 && strcmp(EndParenth, ")") == 0)
        {
            // Make sure that the subset is an expression list
            cbList ExpressionList;
            cbList_Subset(Tokens, &ExpressionList, 2, TokenCount - 3);
            if(cbParse_IsExpressionList(&ExpressionList, SymbolsTable, LineCount, ErrorList))
                return true;
        }
    }
    
    // Else, just check for regular production rules (with the fall-through case)
    char* Operators[] = { "+", "-" };
    return cbParse_IsBinaryProduction(Tokens, SymbolsTable, LineCount, ErrorList, cbParse_IsExpression, cbParse_IsTerm, cbParse_IsTerm, Operators, 2);
}

bool cbParse_IsExpressionList(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Expression list product rule:
    // ExpressionList -> {ExpressionList, Expression | Expression | Empty}
    
    // If empty, that is fine
    if(cbList_GetCount(Tokens) <= 0)
        return true;
    
    // Else, attempt to apply the production rule
    char* Operators[] = { "," };
    return cbParse_IsBinaryProduction(Tokens, SymbolsTable, LineCount, ErrorList, cbParse_IsExpressionList, cbParse_IsExpression, cbParse_IsExpression, Operators, 1);
}

bool cbParse_IsTerm(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Term Product rule:
    // Term -> {Term * Unary | Term / Unary | Term % Unary | Unary}
    
    char* Operators[] = { "*", "/", "%" };
    return cbParse_IsBinaryProduction(Tokens, SymbolsTable, LineCount, ErrorList, cbParse_IsTerm, cbParse_IsUnary, cbParse_IsUnary, Operators, 3);
}

bool cbParse_IsUnary(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Unary Product rule:
    // Unary -> {!Unary | -Unary | Factor}
    
    // Valid if first token is either '!' or '-' and a recursive unary
    char* FirstToken = cbList_PeekFront(Tokens);
    if(strlen(FirstToken) == 1 && (*FirstToken == '!' || *FirstToken == '-'))
    {
        cbList Subset;
        cbList_Subset(Tokens, &Subset, 1, cbList_GetCount(Tokens) - 1);
        if(cbParse_IsUnary(&Subset, SymbolsTable, LineCount, ErrorList))
            return true;
    }
    
    // If it fails, than check if this unary is actually a pure factor
    return cbParse_IsFactor(Tokens, SymbolsTable, LineCount, ErrorList);
}

bool cbParse_IsFactor(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Factor product rule:
    // Factor -> {(bool) | ID | 'true' | 'false' | IntString? | Float | "String Literal"}
    
    // Either is an ID, true or false, an integer, flaot, or string literal
    if(cbList_GetCount(Tokens) == 1)
    {
        char* Token = cbList_PeekFront(Tokens);
        size_t TokenLength = strlen(Token);
        return cbParse_IsID(Token, TokenLength) || cbParse_IsNumString(Token, TokenLength) || cbLang_IsString(Token, TokenLength);
    }
    
    // Check boolean expression (but requires parenth surround)
    char* StartParenth = cbList_PeekFront(Tokens);
    char* EndParenth = cbList_PeekBack(Tokens);
    if(strcmp(StartParenth, "(") == 0 && strcmp(EndParenth, ")") == 0)
    {
        // Pass without the parenth-pair
        cbList Subset;
        cbList_Subset(Tokens, &Subset, 1, cbList_GetCount(Tokens) - 2);
        return cbParse_IsBool(&Subset, SymbolsTable, LineCount, ErrorList);
    }
    
    // Else, all failed
    return false;
}

bool cbParse_IsBool(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Bool product rule:
    // Bool -> {Bool or Join | Join}
    
    char* Operators[] = { "or" };
    return cbParse_IsBinaryProduction(Tokens, SymbolsTable, LineCount, ErrorList, cbParse_IsBool, cbParse_IsJoin, cbParse_IsJoin, Operators, 1);
}

bool cbParse_IsJoin(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Join product rule:
    // Join -> {Join and Equality | Equality}
    
    char* Operators[] = { "and" };
    return cbParse_IsBinaryProduction(Tokens, SymbolsTable, LineCount, ErrorList, cbParse_IsJoin, cbParse_IsEquality, cbParse_IsEquality, Operators, 1);
}

bool cbParse_IsEquality(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Equality product rule:
    // Equality -> {Expression == Expression | Expression != Expression | Expression < Expression | Expression <= Expression | Expression > Expression | Expression >= Expression | Expression}
    
    char* Operators[] = { "==", "!=", "<", "<=", ">", ">=" };
    return cbParse_IsBinaryProduction(Tokens, SymbolsTable, LineCount, ErrorList, cbParse_IsExpression, cbParse_IsExpression, cbParse_IsExpression, Operators, 6);
}

const char* cbParse_GetToken(const char* String, size_t* TokenLength)
{
    // Start and end
    size_t Start, End;
    size_t StringLength = strlen(String);
    
    // Keep skipping white spaces except for new-lines or comments
    for(Start = 0; Start < StringLength; Start++)
        if(String[Start] == '\n' || !isspace(String[Start]))
            break;
    
    // If end of string
    if(String[Start] == '\0' || cbUtil__IsComment(String + Start))
        return NULL;
    
    // If this is a string literal (must be in quotes), seek until next quote
    else if(String[Start] == '"')
    {
        // Keep searching until the end
        for(End = Start + 1; End < StringLength; End++)
        {
            // Grow to include last quote
            if(String[End] == '"')
            {
                End++;
                break;
            }
            else if(String[End] == '\n' || cbUtil__IsComment(String + End))
                break;
        }
    }
    // Else if we hit a boolean operator (of 2-char length)
    else if((StringLength - Start) >= 2 && cbLang_IsOp(String + Start, 2))
    {
        End = Start + 2;
    }
    // Else if we hit a boolean operator (of 1-char length)
    else if((StringLength - Start) >= 1 && cbLang_IsOp(String + Start, 1))
    {
        End = Start + 1;
    }
    // Special seperators delimiters: comma, colon, new-line, and parenth
    else if((StringLength - Start) >= 1 && (String[Start] == ',' || String[Start] == ':' || String[Start] == '\n' || String[Start] == '(' || String[Start] == ')'))
    {
        End = Start + 1;
    }
    // Else, just keep skipping to any sort of non-alphanum character
    else
    {
        // If white space or a special single-char, just stop
        for(End = Start; End < StringLength; End++)
            if(isspace(String[End]) || !isalnum(String[End]) || cbUtil__IsComment(String + Start))
                break;
    }
    
    // Return length and string
    *TokenLength = End - Start;
    return String + Start;
}

bool cbParse_IsBinaryProduction(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList, __cbParse_IsProduct(SymbolA), __cbParse_IsProduct(SymbolB), __cbParse_IsProduct(SymbolC), char** DelimList, size_t DelimCount)
{
    /*
     A helper function to apply production rules to a list of tokens and binary operators. This means
     that this function is an abbstraction that solves the following symbol -> {rules} production resolution
     
     Symbol -> {A * B | A / B | A % B | C}
     
     Note that these operators are only examples, but an arbitrary list of strings can be given of arbitrary
     length. Also note that the "fall-through" rule is optional: if NULL is passed, it is not attempted.
    */
    
    // Number of tokens and default to no known production state
    size_t TokenCount = cbList_GetCount(Tokens);
    bool IsValid = false;
    
    // Must have a minimum of three tokens
    if(TokenCount >= 3)
    {
        // For each possible operator position
        for(int i = 1; i < TokenCount - 1 && !IsValid; i++)
        {
            // Get this token, which if is an op, we split around for a left and right list
            char* Token = cbList_GetElement(Tokens, i);
            
            // For each operator
            for(int j = 0; j < DelimCount && !IsValid; j++)
            {
                // If we have a match...
                if(strcmp(Token, DelimList[j]) == 0)
                {
                    // Generate an A (left) and B (right) list
                    int LeftEnd = i - 1;
                    int RightStart = i + 1;
                    
                    cbList LeftList, RightList;
                    cbList_Subset(Tokens, &LeftList, 0, LeftEnd + 1);
                    cbList_Subset(Tokens, &RightList, RightStart, TokenCount - RightStart);
                    
                    // Validate "Expression + Term" or "Expression - Term"
                    IsValid = SymbolA(&LeftList, SymbolsTable, LineCount, ErrorList) && SymbolB(&RightList, SymbolsTable, LineCount, ErrorList);
                }
            }
        }
    }
    
    // If the calling function defined a "fall-through" symbol, and we don't have
    // a valid product yet, call it
    if(!IsValid && SymbolC != NULL)
        IsValid = SymbolC(Tokens, SymbolsTable, LineCount, ErrorList);
    
    // All done
    return IsValid;
}

void cbParse_RaiseError(cbList* ErrorList, cbError ErrorCode, size_t LineNumber)
{
    // Allocate a new parse-error object
    cbParseError* NewError = malloc(sizeof(cbParseError));
    NewError->ErrorCode = ErrorCode;
    NewError->LineNumber = LineNumber;
    
    // Insert into list
    cbList_PushBack(ErrorList, NewError);
}
