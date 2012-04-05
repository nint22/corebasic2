/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#include "cbParse.h"

bool cbParse_ParseProgram(const char* Program, cbList* ErrorList, cbSymbolsTable* SymbolsTable)
{
    // Create our symbols table (just for lexical analysis help for now)
    SymbolsTable->BlockDepth = 0;
    cbList_Init(&SymbolsTable->LexTree);
    
    // Keep an active token pointer; this changes over time
    const char* ActiveLine = Program;
    
    // The line we are on
    size_t LineCount = 1;
    
    // Keep parsing until done
    while(true)
    {
        // Parse this line
        cbLexNode* LexTree = cbParse_ParseLine(ActiveLine, SymbolsTable, LineCount, ErrorList);
        if(LexTree != NULL)
            cbList_PushBack(&SymbolsTable->LexTree, LexTree);
        
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
    if(SymbolsTable->BlockDepth > 0)
        cbParse_RaiseError(ErrorList, cbError_BlockMismatch, LineCount);
    
    // Done parsing, return the symbols table
    return cbList_GetCount(ErrorList) <= 0;
}

cbLexNode* cbParse_ParseLine(const char* Line, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
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
    
    // Our parsed line
    cbLexNode* LexTree = NULL;
    
    // Only process if there are tokens
    if(cbList_GetCount(&Tokens) > 0)
    {
        // Line production rule:
        // Line -> {Statement | Declaration}, but if we have an active conditional stack, validate elif, else, and end product ruels
        LexTree = cbParse_IsDeclaration(&Tokens, LineCount, ErrorList);
        if(LexTree == NULL)
            LexTree = cbParse_IsStatement(&Tokens, SymbolsTable, LineCount, ErrorList);
        
        // On error
        if(LexTree == NULL)
            cbParse_RaiseError(ErrorList, cbError_UnknownLine, LineCount);
        
        // Release the tokens
        while(cbList_GetCount(&Tokens) > 0)
            free(cbList_PopFront(&Tokens));
    }
    
    // All done
    return LexTree;
}

cbLexNode* cbParse_IsStatement(cbList* Tokens, cbSymbolsTable* SymbolsTable, size_t LineCount, cbList* ErrorList)
{
    // Statement production rule:
    // Statement -> {StatementIf? | StatementWhile? | StatementFor? | StatementGoto? | StatementLabel? | Expression}
    cbLexNode* Node = NULL;
    
    // Note: Order is important, because if we don't check for if and while before checking for functions, the
    // expression product will sudgest that a "while(something)" looks like a function call
    
    // Starts new blocks
    if((Node = cbParse_IsStatementIf(Tokens, LineCount, ErrorList)) || (Node = cbParse_IsStatementWhile(Tokens, LineCount, ErrorList)) || (Node = cbParse_IsStatementFor(Tokens, LineCount, ErrorList)))
    {
        SymbolsTable->BlockDepth++;
        return Node;
    }
    
    // Must have at least one block active
    if(SymbolsTable->BlockDepth > 0 && (Node = cbParse_IsStatementElif(Tokens, LineCount, ErrorList)))
        return Node;
    if(SymbolsTable->BlockDepth > 0 && (Node = cbParse_IsStatementElse(Tokens, LineCount, ErrorList)))
        return Node;
    if(SymbolsTable->BlockDepth > 0 && (Node = cbParse_IsStatementEnd(Tokens, LineCount, ErrorList)))
    {
        // Can never close non-existing blocks
        if(SymbolsTable->BlockDepth <= 0)
            cbParse_RaiseError(ErrorList, cbError_BlockMismatch, LineCount);
        else
            SymbolsTable->BlockDepth--;
        return Node;
    }
    
    // Regular / simple production rules
    if((Node = cbParse_IsStatementGoto(Tokens, LineCount, ErrorList)))
        return Node;
    if((Node = cbParse_IsStatementLabel(Tokens, LineCount, ErrorList)))
        return Node;
    if((Node = cbParse_IsExpression(Tokens, LineCount, ErrorList)))
        return Node;
    
    // If matched, pop off stack since we are progressing back up, else, delete the node
    return Node;
}

cbLexNode* cbParse_IsDeclaration(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Declaration production rule:
    // Declaration -> {ID = Expression}
    cbLexNode* Node = NULL;
    
    // Must have a minimum of three or more tokens
    if(cbList_GetCount(Tokens) >= 3)
    {
        // First token must always be an ID
        char* DestID = cbList_GetElement(Tokens, 0);
        char* AssignmentOp = cbList_GetElement(Tokens, 1);
        
        if(cbParse_IsID(DestID, strlen(DestID)) && strcmp(AssignmentOp, "=") == 0)
        {
            // Create node for this symbol
            Node = cbLex_CreateNodeSymbol(cbSymbol_Declaration, LineCount);
            
            // Save ID and op into the parse-tree
            Node->Left = cbLex_CreateNodeV(DestID, LineCount);
            Node->Middle = cbLex_CreateNodeO(cbOps_Set, LineCount);
            
            // The rest is assumed an expression
            cbList ExpressionTokens;
            cbList_Subset(Tokens, &ExpressionTokens, 2, cbList_GetCount(Tokens) - 2);
            
            // Verify as sub-expression
            Node->Right = cbParse_IsExpression(&ExpressionTokens, LineCount, ErrorList);
            if(Node->Right == NULL)
            {
                cbParse_RaiseError(ErrorList, cbError_Assignment, LineCount);
                cbLex_DeleteNode(&Node);
            }
        }
    }
    
    // Failed
    return Node;
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

cbLexNode* cbParse_IsStatementIf(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // If production rule:
    // StatementIf -> {if(Bool) Lines end | if(Bool) Lines StatementElif? | if(Bool) Lines StatementElse?}
    return cbParse_IsKeywordBoolProduction(Tokens, LineCount, ErrorList, "if", cbSymbol_StatementIf);
}

cbLexNode* cbParse_IsStatementElif(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Elif production rule:
    // StatementElif -> {elif(Bool) Lines end | elif(Bool) Lines StatementElif? | elif(Bool) Lines StatementElse?}
    return cbParse_IsKeywordBoolProduction(Tokens, LineCount, ErrorList, "elif", cbSymbol_StatementElif);
}

cbLexNode* cbParse_IsStatementElse(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Else production rule:
    // StatementElse -> {else Lines end}
    return cbParse_IsKeywordProduction(Tokens, LineCount, ErrorList, "else", cbSymbol_StatementElse);
}

cbLexNode* cbParse_IsStatementEnd(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Else production rule:
    // StatementElse -> {else Lines end}
    return cbParse_IsKeywordProduction(Tokens, LineCount, ErrorList, "end", cbSymbol_End);
}

cbLexNode* cbParse_IsStatementWhile(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // While production rule:
    // StatementWhile -> {while(Bool) Lines end}
    return cbParse_IsKeywordBoolProduction(Tokens, LineCount, ErrorList, "while", cbSymbol_StatementWhile);
}

cbLexNode* cbParse_IsStatementFor(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // For production rule:
    // StatementFor -> {for(ID, Expression, Expression, Expression) Lines end}
    
    // TODO...
    return false;
}

cbLexNode* cbParse_IsStatementGoto(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Should be of form "goto <label name>"
    cbLexNode* Node = NULL;
    
    // Must be two tokens, with the first being "goto"
    if(cbList_GetCount(Tokens) == 2 && strcmp(cbList_GetElement(Tokens, 0), "goto") == 0)
    {
        // Second must always be an ID
        char* ID = cbList_GetElement(Tokens, 1);
        if(cbParse_IsID(ID, strlen(ID)))
        {
            Node = cbLex_CreateNodeSymbol(cbSymbol_StatementGoto, LineCount);
            Node->Middle = cbLex_CreateNodeS(ID, LineCount);
        }
        else
            cbParse_RaiseError(ErrorList, cbError_InvalidID, LineCount);
    }
    
    return Node;
}

cbLexNode* cbParse_IsStatementLabel(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Should be of form "label <label name>:"
    cbLexNode* Node = NULL;
    
    // Must be two tokens, with the first being "goto"
    if(cbList_GetCount(Tokens) == 3 && strcmp(cbList_GetElement(Tokens, 0), "label") == 0 && strcmp(cbList_GetElement(Tokens, 2), ":") == 0)
    {
        // Second must always be an ID
        char* ID = cbList_GetElement(Tokens, 1);
        if(cbParse_IsID(ID, strlen(ID)))
        {
            Node = cbLex_CreateNodeSymbol(cbSymbol_StatementLabel, LineCount);
            Node->Middle = cbLex_CreateNodeS(ID, LineCount);
        }
        else
            cbParse_RaiseError(ErrorList, cbError_InvalidID, LineCount);
    }
    
    return Node;
}

cbLexNode* cbParse_IsExpression(cbList* Tokens, size_t LineCount, cbList* ErrorList)
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
    cbLexNode* Node = NULL;
    
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
            // Function call tree element
            Node = cbLex_CreateNodeO(cbOps_Func, LineCount);
            
            // Make sure that the subset is an expression list
            cbList ExpressionList;
            cbList_Subset(Tokens, &ExpressionList, 2, TokenCount - 3);
            Node->Middle = cbParse_IsExpressionList(&ExpressionList, LineCount, ErrorList);
        }
    }
    
    // If not yet succeeded
    if(Node == NULL)
    {
        // Else, just check for regular production rules (with the fall-through case)
        char* Operators[] = { "+", "-" };
        Node = cbParse_IsBinaryProduction(Tokens, LineCount, ErrorList, cbParse_IsExpression, cbParse_IsTerm, cbParse_IsTerm, Operators, 2);
    }
    
    return Node;
}

cbLexNode* cbParse_IsExpressionList(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Expression list product rule:
    // ExpressionList -> {ExpressionList, Expression | Expression | Empty}
    cbLexNode* Node = NULL;
    
    // If empty, that is fine, but we don't save the symbol
    if(cbList_GetCount(Tokens) <= 0)
        Node = cbLex_CreateNode(cbSymbol_None, LineCount); // Empty
    // Else, attempt to apply the production rule
    else
    {
        char* Operators[] = { "," };
        Node = cbParse_IsBinaryProduction(Tokens, LineCount, ErrorList, cbParse_IsExpressionList, cbParse_IsExpression, cbParse_IsExpression, Operators, 1);
    }
    
    return Node;
}

cbLexNode* cbParse_IsTerm(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Term Product rule:
    // Term -> {Term * Unary | Term / Unary | Term % Unary | Unary}
    char* Operators[] = { "*", "/", "%" };
    return cbParse_IsBinaryProduction(Tokens, LineCount, ErrorList, cbParse_IsTerm, cbParse_IsUnary, cbParse_IsUnary, Operators, 3);
}

cbLexNode* cbParse_IsUnary(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Unary Product rule:
    // Unary -> {!Unary | -Unary | Factor}
    cbLexNode* Node = NULL;
    
    // Valid if first token is either '!' or '-' and a recursive unary
    char* FirstToken = cbList_PeekFront(Tokens);
    if(strlen(FirstToken) == 1 && (*FirstToken == '!' || *FirstToken == '-'))
    {
        cbList Subset;
        cbList_Subset(Tokens, &Subset, 1, cbList_GetCount(Tokens) - 1);
        Node = cbParse_IsUnary(&Subset, LineCount, ErrorList);
    }
    
    // If it fails, than check if this unary is actually a pure factor
    if(Node == NULL)
        Node = cbParse_IsFactor(Tokens, LineCount, ErrorList);
    
    return Node;
}

cbLexNode* cbParse_IsFactor(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Factor product rule:
    // Factor -> {(bool) | ID | 'true' | 'false' | IntString? | Float | "String Literal"}
    cbLexNode* Node = NULL;
    
    // Either is an ID, true or false, an integer, flaot, or string literal
    if(cbList_GetCount(Tokens) == 1)
    {
        char* Token = cbList_PeekFront(Tokens);
        size_t TokenLength = strlen(Token);
        
        // Boolean (first since it could be seen as a variable)
        if(cbLang_IsBoolean(Token, TokenLength))
            Node = cbLex_CreateNodeB((Token[0] == 't') ? true : false, LineCount);
        // Variable
        else if(cbParse_IsID(Token, TokenLength))
            Node = cbLex_CreateNodeV(Token, LineCount);
        // String
        else if(cbLang_IsString(Token, TokenLength))
            Node = cbLex_CreateNodeS(Token, LineCount);
        // Float
        else if(cbLang_IsFloat(Token, TokenLength))
        {
            float Val;
            sscanf(Token, "%f", &Val);
            Node = cbLex_CreateNodeF(Val, LineCount);
        }
        // Integer
        else if(cbLang_IsInteger(Token, TokenLength))
        {
            int Val;
            sscanf(Token, "%d", &Val);
            Node = cbLex_CreateNodeI(Val, LineCount);
        }
        // Else, unknown
        else
            cbParse_RaiseError(ErrorList, cbError_UnknownToken, LineCount);
    }
    else
    {
        // Check boolean expression (but requires parenth surround)
        char* StartParenth = cbList_PeekFront(Tokens);
        char* EndParenth = cbList_PeekBack(Tokens);
        if(strcmp(StartParenth, "(") == 0 && strcmp(EndParenth, ")") == 0)
        {
            // Pass without the parenth-pair
            cbList Subset;
            cbList_Subset(Tokens, &Subset, 1, cbList_GetCount(Tokens) - 2);
            Node = cbParse_IsBool(&Subset, LineCount, ErrorList);
        }
    }
    
    return Node;
}

cbLexNode* cbParse_IsBool(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Bool product rule:
    // Bool -> {Bool or Join | Join}
    char* Operators[] = { "or" };
    return cbParse_IsBinaryProduction(Tokens, LineCount, ErrorList, cbParse_IsBool, cbParse_IsJoin, cbParse_IsJoin, Operators, 1);
}

cbLexNode* cbParse_IsJoin(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Join product rule:
    // Join -> {Join and Equality | Equality}
    char* Operators[] = { "and" };
    return cbParse_IsBinaryProduction(Tokens, LineCount, ErrorList, cbParse_IsJoin, cbParse_IsEquality, cbParse_IsEquality, Operators, 1);
}

cbLexNode* cbParse_IsEquality(cbList* Tokens, size_t LineCount, cbList* ErrorList)
{
    // Equality product rule:
    // Equality -> {Expression == Expression | Expression != Expression | Expression < Expression | Expression <= Expression | Expression > Expression | Expression >= Expression | Expression}
    char* Operators[] = { "==", "!=", "<", "<=", ">", ">=" };
    return cbParse_IsBinaryProduction(Tokens, LineCount, ErrorList, cbParse_IsExpression, cbParse_IsExpression, cbParse_IsExpression, Operators, 6);
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
    if(String[Start] == '\0' || cbUtil_IsComment(String + Start))
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
            else if(String[End] == '\n' || cbUtil_IsComment(String + End))
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
            if(isspace(String[End]) || !isalnum(String[End]) || cbUtil_IsComment(String + Start))
                break;
    }
    
    // Return length and string
    *TokenLength = End - Start;
    return String + Start;
}

cbLexNode* cbParse_IsKeywordProduction(cbList* Tokens, size_t LineCount, cbList* ErrorList,  const char* Keyword, cbSymbol Symbol)
{
    // Single keyword
    if(cbList_GetCount(Tokens) == 1)
    {
        char* Token = cbList_PeekFront(Tokens);
        if(strcmp(Token, Keyword) == 0)
            return cbLex_CreateNodeSymbol(Symbol, LineCount);
    }
    
    // Failed
    return NULL;
}

cbLexNode* cbParse_IsKeywordBoolProduction(cbList* Tokens, size_t LineCount, cbList* ErrorList, const char* Keyword, cbSymbol Symbol)
{
    // If production rule: a -> {Keyword(Bool)}
    cbLexNode* Node = NULL;
    
    // If conditions always start with keyword, (, <bool>, ) ..
    if(cbList_GetCount(Tokens) >= 4)
    {
        // First token must be the keyword and second and last token should always be '(' and ')' respectivly
        char* IfToken = cbList_GetElement(Tokens, 0);
        char* FirstParenth = cbList_GetElement(Tokens, 1);
        char* LastParenth = cbList_PeekBack(Tokens);
        if(strcmp(IfToken, Keyword) == 0 && strcmp(FirstParenth, "(") == 0 && strcmp(LastParenth, ")") == 0)
        {
            // Valid keyword, save the boolean expression within self
            Node = cbLex_CreateNodeSymbol(Symbol, LineCount);
            
            // Pase the boolean expression within the parenth
            cbList BoolSubset;
            cbList_Subset(Tokens, &BoolSubset, 2, cbList_GetCount(Tokens) - 3);
            
            // Boolean expression must be valid
            Node->Middle = cbParse_IsBool(&BoolSubset, LineCount, ErrorList);
        }
    }
    
    return Node;
}

cbLexNode* cbParse_IsBinaryProduction(cbList* Tokens, size_t LineCount, cbList* ErrorList, __cbParse_IsProduct(SymbolA), __cbParse_IsProduct(SymbolB), __cbParse_IsProduct(SymbolC), char** DelimList, size_t DelimCount)
{
    /*
     A helper function to apply production rules to a list of tokens and binary operators. This means
     that this function is an abbstraction that solves the following symbol -> {rules} production resolution
     
     Symbol -> {A * B | A / B | A % B | C}
     
     Note that these operators are only examples, but an arbitrary list of strings can be given of arbitrary
     length. Also note that the "fall-through" rule is optional: if NULL is passed, it is not attempted.
    */
    cbLexNode* Node = NULL;
    
    // Must have a minimum of three tokens
    size_t TokenCount = cbList_GetCount(Tokens);
    if(TokenCount >= 3)
    {
        // For each possible operator position
        for(int i = 1; i < TokenCount - 1 && Node == NULL; i++)
        {
            // Get this token, which if is an op, we split around for a left and right list
            char* Token = cbList_GetElement(Tokens, i);
            
            // For each operator
            for(int j = 0; j < DelimCount && Node == NULL; j++)
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
                    cbLexNode* LeftProduct = SymbolA(&LeftList, LineCount, ErrorList);
                    cbLexNode* RightProduct = SymbolB(&RightList, LineCount, ErrorList);
                    
                    // If both are valid and we have an op, form a node
                    cbOps Op;
                    bool ValidOp = cbUtil_OpFromStr(DelimList[j], &Op);
                    if(LeftProduct != NULL && RightProduct != NULL && ValidOp)
                    {
                        Node = cbLex_CreateNodeO(Op, LineCount);
                        Node->Left = LeftProduct;
                        Node->Right = RightProduct;
                    }
                    // Release left or right on failure
                    else
                    {
                        if(LeftProduct != NULL)
                            cbLex_DeleteNode(&LeftProduct);
                        if(RightProduct != NULL)
                            cbLex_DeleteNode(&RightProduct);
                    }
                }
            }
        }
    }
    
    // If the calling function defined a "fall-through" symbol, and we don't have
    // a valid product yet, call it
    if(Node == NULL && SymbolC != NULL)
        Node = SymbolC(Tokens, LineCount, ErrorList);
    
    // All done
    return Node;
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

cbLexNode* cbLex_CreateNode(cbLexNodeType Type, size_t LineNumber)
{
    cbLexNode* Node = malloc(sizeof(cbLexNode));
    Node->Type = Type;
    Node->Left = Node->Middle = Node->Right = NULL;
    Node->LineNumber = LineNumber;
    return Node;
}

cbLexNode* cbLex_CreateNodeSymbol(cbSymbol Symbol, size_t LineNumber)
{
    cbLexNode* Node = cbLex_CreateNode(cbLexNodeType_Symbol, LineNumber);
    Node->Data.Symbol = Symbol;
    return Node;
}

cbLexNode* cbLex_CreateNodeI(int Integer, size_t LineNumber)
{
    cbLexNode* Node = cbLex_CreateNode(cbLexNodeType_Terminal, LineNumber);
    Node->Data.Terminal.Type = cbLexIDType_Int;
    Node->Data.Terminal.Data.Integer = Integer;
    return Node;
}

cbLexNode* cbLex_CreateNodeF(float Float, size_t LineNumber)
{
    cbLexNode* Node = cbLex_CreateNode(cbLexNodeType_Terminal, LineNumber);
    Node->Data.Terminal.Type = cbLexIDType_Float;
    Node->Data.Terminal.Data.Float = Float;
    return Node;
}

cbLexNode* cbLex_CreateNodeB(bool Boolean, size_t LineNumber)
{
    cbLexNode* Node = cbLex_CreateNode(cbLexNodeType_Terminal, LineNumber);
    Node->Data.Terminal.Type = cbLexIDType_Bool;
    Node->Data.Terminal.Data.Boolean = Boolean;
    return Node;
}

cbLexNode* cbLex_CreateNodeS(const char* StringLiteral, size_t LineNumber)
{
    cbLexNode* Node = cbLex_CreateNode(cbLexNodeType_Terminal, LineNumber);
    Node->Data.Terminal.Type = cbLexIDType_StringLit;
    Node->Data.Terminal.Data.String = cbUtil_stralloc(StringLiteral);
    return Node;
}

cbLexNode* cbLex_CreateNodeV(const char* VariableName, size_t LineNumber)
{
    cbLexNode* Node = cbLex_CreateNode(cbLexNodeType_Terminal, LineNumber);
    Node->Data.Terminal.Type = cbLexIDType_Variable;
    Node->Data.Terminal.Data.String = cbUtil_stralloc(VariableName);
    return Node;
}

cbLexNode* cbLex_CreateNodeO(cbOps Op, size_t LineNumber)
{
    cbLexNode* Node = cbLex_CreateNode(cbLexNodeType_Terminal, LineNumber);
    Node->Data.Terminal.Type = cbLexIDType_Op;
    Node->Data.Terminal.Data.Op = Op;
    return Node;
}

void cbLex_DeleteNode(cbLexNode** Node)
{
    // If null, ignore
    if(Node == NULL || *Node == NULL)
        return;
    
    // Go bottom up: remove children first, then self
    cbLex_DeleteNode(&(*Node)->Left);
    cbLex_DeleteNode(&(*Node)->Middle);
    cbLex_DeleteNode(&(*Node)->Right);
    
    // Release data
    if((*Node)->Type == cbLexNodeType_Terminal)
    {
        if((*Node)->Data.Terminal.Type == cbLexIDType_StringLit || (*Node)->Data.Terminal.Type == cbLexIDType_Variable)
            free((*Node)->Data.Terminal.Data.String);
    }
    
    // Release self and set to NULL
    free(*Node);
    *Node = NULL;
}
