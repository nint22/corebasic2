/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#include "cbParse.h"

void cbParse_ParseProgram(const char* Program, cbList* ErrorList)
{
    // Keep an active token pointer; this changes over time
    const char* ActiveToken = Program;
    size_t TokenLength = 0;
    
    // The line we are on
    size_t LineCount = 1;
    
    // Keep parsing until done
    while(true)
    {
        // Get the active token (previous address + previous length)
        ActiveToken = cbParse_GetToken(ActiveToken + TokenLength, &TokenLength);
        
        // If this is the end of the stream, or the token has no length
        if(ActiveToken == NULL)
            break;
        
        // If this is a newline, we grow the line count
        else if(TokenLength == 1 && ActiveToken[0] == '\n')
            LineCount++;
        
        // Regular token to parse
        else
        {
            // Apply production rules
            
        }
    }
    
    // Done parsing
}

/*void cbParse_ParseLine(const char* Line, cbList* ErrorList)
{
    
}
*/
void cbParse_CompileProgram(cbList* ErrorList)
{
    
}


/**** OLD CODE ****/

void cbParse_RaiseError(cbList* ErrorList, cbError ErrorCode, size_t LineNumber)
{
    // Allocate a new parse-error object
    cbParseError* NewError = malloc(sizeof(cbParseError));
    NewError->ErrorCode = ErrorCode;
    NewError->LineNumber = LineNumber;
    
    // Insert into list
    cbList_PushBack(ErrorList, NewError);
}

cbError cbParse_ParseBlock(char** Code, cbList* InstructionsList, cbList* DataList, cbList* VariablesList, cbList* JumpTable, cbList* LabelTable, int StackDepth)
{
    // Define an error flag we can set over time
    cbError Error = cbError_None;
    
    /*** Stack Frame Vars ***/
    
    // The for-loop header, after the initialization
    cbInstruction* ActiveFor = NULL;
    
    // The conditional headers
    cbList ConditionalJumps;
    cbList_Init(&ConditionalJumps);
    
    // Previous conditional we were working on
    cbInstruction* PrevConditional = NULL;
    
    /*** Line Parsing ***/
    
    // Save the last address, just incase we try to read past that
    char* LastCode = *Code + strlen(*Code);
    
    // Keep parsing until there is an error or we are done reading each line
    while(Error == cbError_None && *Code != NULL && *Code < LastCode)
    {
        /*** Prep. Lines ***/
        
        // Grab the first line
        char* Line = strtok(*Code, "\r\n");
        if(Line == NULL)
            break;
        else
            *Code += strlen(*Code) + 1;
        
        // If empty line, ignore
        bool IsEmpty = true;
        for(int i = 0; i < strlen(Line) && IsEmpty; i++)
        {
            // Is empty, give up
            if(isgraph(Line[i]))
                IsEmpty = false;
        }
        
        if(IsEmpty)
            continue;
        
        // Get the first token of this string
        size_t FirstTokenLength;
        const char* FirstToken = cbParse_GetToken(Line, &FirstTokenLength);
        
        /*** Special Built-In Functions ***/
        
        // If while loop
        if(strncmp(FirstToken, "while", FirstTokenLength) == 0)
        {
            // Save the top of the load frame for the conditional
            size_t LoadIndex = cbList_GetCount(InstructionsList);
            
            // Save the address of the while instruction itself
            cbParse_ParseLine(Line, InstructionsList, DataList, VariablesList, JumpTable);
            cbInstruction* WhileOp = cbList_PeekBack(InstructionsList);
            if(WhileOp->Op != cbOps_While)
            {
                Error = cbError_UnknownOp;
                break;
            }
            else
                WhileOp->Op = cbOps_If;
            
            // Parse block
            Error = cbParse_ParseBlock(Code, InstructionsList, DataList, VariablesList, JumpTable, LabelTable, StackDepth + 1);
            if(Error != cbError_None)
                break;
            
            // Jump to the top of the while load frame
            cbInstruction* JumpOp = malloc(sizeof(cbInstruction));
            JumpOp->Op = cbOps_Goto;
            JumpOp->Arg = (int)(LoadIndex - cbList_GetCount(InstructionsList)) * sizeof(cbInstruction);
            cbList_PushBack(InstructionsList, JumpOp);
            
            // Have the while-op jump to here on failure
            WhileOp->Arg = (int)(cbList_GetCount(InstructionsList) - cbList_FindOffset(InstructionsList, WhileOp, cbList_ComparePointer)) * sizeof(cbInstruction);
        }
        // Is this the "for" function?
        else if(strncmp(FirstToken, "for", FirstTokenLength) == 0)
        {
            /*** Start of For Loop Parsing ***/
            
            // The working variable and increment for the for-loop function
            // Load all four arguments from this for loop
            char *IteratorExp = NULL, *MinExp = NULL, *MaxExp = NULL, *IncrementExp = NULL;
            if((Error = cbParse_ParseFor(Line, &IteratorExp, &MinExp, &MaxExp, &IncrementExp)) != cbError_None)
                break;
            
            // Initialize variable with initial value argument
            if((Error = cbLang_LoadVariable(InstructionsList, VariablesList, IteratorExp, strlen(IteratorExp))) != cbError_None)
                break;
            if((Error = cbParse_ParseLine(MinExp, InstructionsList, DataList, VariablesList, JumpTable)) != cbError_None)
                break;
            if((Error = cbLang_LoadOp(InstructionsList, "=", 1)) != cbError_None)
                break;
            
            // Save this as the instruction we need to jump back to
            int LoadForJump = (int)cbList_GetCount(InstructionsList);
            
            // Conditional
            if((Error = cbLang_LoadVariable(InstructionsList, VariablesList, IteratorExp, strlen(IteratorExp))) != cbError_None)
                break;
            if((Error = cbParse_ParseLine(MaxExp, InstructionsList, DataList, VariablesList, JumpTable)) != cbError_None)
                break;
            if((Error = cbLang_LoadOp(InstructionsList, "<=", 2)) != cbError_None)
                break;
            
            // If op (to check conditional)
            ActiveFor = malloc(sizeof(cbInstruction));
            ActiveFor->Op = cbOps_If;
            ActiveFor->Arg = -1; // Needs to be set later on
            cbList_PushBack(InstructionsList, ActiveFor);
            
            /*** Parse Internal Block ***/
            
            // Parse this internal block
            Error = cbParse_ParseBlock(Code, InstructionsList, DataList, VariablesList, JumpTable, LabelTable, StackDepth + 1);
            if(Error != cbError_None)
                break;
            
            /*** End of For Loop Parsing ***/
            
            // Now that we are at the end, grow the variable, and jump
            if((Error = cbLang_LoadVariable(InstructionsList, VariablesList, IteratorExp, strlen(IteratorExp))) != cbError_None)
                break;
            if((Error = cbLang_LoadVariable(InstructionsList, VariablesList, IteratorExp, strlen(IteratorExp))) != cbError_None)
                break;
            if((Error = cbParse_ParseLine(IncrementExp, InstructionsList, DataList, VariablesList, JumpTable)) != cbError_None)
                break;
            if((Error = cbLang_LoadOp(InstructionsList, "+", 1)) != cbError_None)
                break;
            if((Error = cbLang_LoadOp(InstructionsList, "=", 1)) != cbError_None)
                break;
            
            // Have the conditional jump correctly, and the jump go up
            int ForBottom = (int)cbList_GetCount(InstructionsList);
            
            // Jump to the top as needed
            cbInstruction* JumpTop = malloc(sizeof(cbInstruction));
            JumpTop->Op = cbOps_Goto;
            JumpTop->Arg = -(ForBottom - LoadForJump) * sizeof(cbInstruction);
            cbList_PushBack(InstructionsList, JumpTop);
            
            // Jump down to the correct address
            int ForIndex = cbList_FindOffset(InstructionsList, ActiveFor, cbList_ComparePointer);
            ActiveFor->Arg = (ForBottom - ForIndex + 1) * sizeof(cbInstruction);
            
            // Done with this for-loop
            ActiveFor = NULL;
            
            // Release for-loop header details
            free(MinExp);
            free(MaxExp);
            free(IteratorExp);
            free(IncrementExp);
        }
        else if(strncmp(FirstToken, "if", FirstTokenLength) == 0)
        {
            // Fork if this is a new conditional start
            if(PrevConditional != NULL)
            {
                // Put the line back into the code before starting a recursive call
                int LineLength = (int)strlen(Line);
                int Overlap = (int)(Line - *Code);
                *Code += Overlap;
                for(int i = LineLength; i < abs(Overlap); i++)
                    (*Code)[i] = '\n';
                
                // Start new conditional group
                Error = cbParse_ParseBlock(Code, InstructionsList, DataList, VariablesList, JumpTable, LabelTable, StackDepth + 1);
            }
            // Else, all good
            else
            {
                // Parse conditional and retain the conditional
                Error = cbParse_ParseLine(Line, InstructionsList, DataList, VariablesList, JumpTable);
                PrevConditional = cbList_PeekBack(InstructionsList);
            }
        }
        else if(strncmp(FirstToken, "elif", FirstTokenLength) == 0)
        {
            // Fail out if the conditional list is empty (i.e. we never started with an if)
            if(PrevConditional == NULL)
            {
                Error = cbError_BlockMismatch;
                break;
            }
            else
            {
                // Add jump to the end, track it
                cbInstruction* JumpOp = malloc(sizeof(cbInstruction));
                JumpOp->Op = cbOps_Goto;
                cbList_PushBack(InstructionsList, JumpOp);
                cbList_PushBack(&ConditionalJumps, JumpOp);
                
                // Set the previous conditional to this pre-loading if-frame
                int PrevConIndex = cbList_FindOffset(InstructionsList, PrevConditional, cbList_ComparePointer);
                PrevConditional->Arg = ((int)cbList_GetCount(InstructionsList) - PrevConIndex) * sizeof(cbInstruction);
                
                // Parse the conditional (change elif to if)
                Error = cbParse_ParseLine(Line, InstructionsList, DataList, VariablesList, JumpTable);
                
                // Change else to if, if needed
                PrevConditional = cbList_PeekBack(InstructionsList);
                if(PrevConditional->Op == cbOps_Elif)
                {
                    PrevConditional->Op = cbOps_If;
                }
                // Else, fail, should always be a conditional
                else
                {
                    Error = cbError_BlockMismatch;
                    break; 
                }
            }
        }
        else if(strncmp(FirstToken, "else", FirstTokenLength) == 0)
        {
            // Must be paired with previous conditional
            if(PrevConditional == NULL)
            {
                Error = cbError_BlockMismatch;
                break;
            }
            else
            {
                // Set the previous instruction jump to this address if false, then reset conditional to null
                // Note: -1 because we don't have an op in this case with the "else"
                int PrevConIndex = cbList_FindOffset(InstructionsList, PrevConditional, cbList_ComparePointer);
                PrevConditional->Arg = ((int)cbList_GetCount(InstructionsList) - PrevConIndex + 1) * sizeof(cbInstruction);
                PrevConditional = NULL;
                
                // Add jump to the end, track it
                cbInstruction* JumpOp = malloc(sizeof(cbInstruction));
                JumpOp->Op = cbOps_Goto;
                cbList_PushBack(InstructionsList, JumpOp);
                cbList_PushBack(&ConditionalJumps, JumpOp);
            }
        }
        
        /*** End of Block ***/
        
        // Else, end of our conditional blocks
        else if(strncmp(FirstToken, "end", FirstTokenLength) == 0)
        {
            // If we were working on a block of conditionals, make sure to wrap the last conditional
            if(PrevConditional != NULL)
            {
                // Set the previous instruction jump to this address if false, then reset conditional to null
                int PrevConIndex = cbList_FindOffset(InstructionsList, PrevConditional, cbList_ComparePointer);
                PrevConditional->Arg = ((int)cbList_GetCount(InstructionsList) - PrevConIndex) * sizeof(cbInstruction);
                PrevConditional = NULL;
            }
            
            // Wrap up any dangling pointers to this position (end of conditionals)
            cbInstruction* JumpInstr = NULL;
            while((JumpInstr = cbList_PopBack(&ConditionalJumps)) != NULL)
            {
                // The address of this jump instruction
                int Top = cbList_FindOffset(InstructionsList, JumpInstr, cbList_ComparePointer);
                
                // The current bottom address
                int Bottom = (int)cbList_GetCount(InstructionsList);
                
                // Set to jump down
                JumpInstr->Arg = (Bottom - Top) * sizeof(cbInstruction);
            }
            
            // Done if we are pushed on the stack, else if we are the base just continue parsing
            if(StackDepth > 0)
                break;
        }
        
        /*** Regular Jump / Label ***/
        
        else if(strncmp(FirstToken, "goto", FirstTokenLength) == 0)
        {
            // Copy the rest of the arguments (label name)
            char* LabelName = malloc(strlen(FirstToken + FirstTokenLength + 1) + 1);
            strcpy(LabelName, FirstToken + FirstTokenLength + 1);
            
            // Create the jump instruction and save it in the instructions and jump table
            cbInstruction* JumpOp = malloc(sizeof(cbInstruction));
            JumpOp->Op = cbOps_Goto;
            JumpOp->Arg = -1;
            cbList_PushBack(InstructionsList, JumpOp);
            
            // Save into the jump table, though warning: this table is actually a
            // mixed array of 0: jump operator, then 1: label-name (from front-to-back)
            cbList_PushFront(JumpTable, LabelName);
            cbList_PushFront(JumpTable, JumpOp);
        }
        
        else if(strncmp(FirstToken, "label", FirstTokenLength) == 0)
        {
            // Create label handle
            __cbLabel* Label = malloc(sizeof(__cbLabel));
            
            // Save index and copy string
            Label->Index = (int)cbList_GetCount(InstructionsList) - 1;
            strcpy(Label->Label, FirstToken + FirstTokenLength + 1);
            
            // Remove any dead space behind the label (including the optional ':')
            size_t LabelLength = strlen(Label->Label);
            for(size_t i = LabelLength - 1; i > 0; i--)
            {
                if(isgraph(Label->Label[i]))
                    break;
                else
                    Label->Label[i] = 0;
            }
            
            // Save in the labels list
            cbList_PushBack(LabelTable, Label);
        }
        
        /*** Regular Parsing ***/
        
        else
        {
            // Parse the line, adding whatever is needed
            Error = cbParse_ParseLine(Line, InstructionsList, DataList, VariablesList, JumpTable);
        }
    }
    
    // Write all jumps to this location
    cbInstruction* JumpInstr = NULL;
    while((JumpInstr = cbList_PopBack(&ConditionalJumps)) != NULL)
        JumpInstr->Arg = (int)cbList_GetCount(&ConditionalJumps);
    cbList_Release(&ConditionalJumps);
    
    // Return any error
    return Error;
}

cbError cbParse_ParseLine(char* Line, cbList* InstructionsList, cbList* DataList, cbList* VariablesList, cbList* JumpTable)
{
    // Ignore if null
    if(Line == NULL)
        return cbError_Null;
    
    // Parse from left to right into a RPN list of heap-allocated strings
    cbList OpsList;
    cbError Error = cbParse_ParseExpression(Line, &OpsList);
    
    // For each element in this list, from left to right, parse as
    // given reverse polish notation into executable code
    char* Token = NULL;
    for(size_t TokenCount = 0; (Token = cbList_PopFront(&OpsList)) != NULL && Error == cbError_None; TokenCount++)
    {
        // Get token length
        size_t TokenLength = strlen(Token);
        
        /*** Operators ***/
        
        // If operator, call it
        if(cbLang_IsOp(Token, strlen(Token)))
        {
            Error = cbLang_LoadOp(InstructionsList, Token, TokenLength);
        }
        
        // If function, call it
        else if(cbLang_IsFunction(Token, TokenLength))
        {
            // Add instruction into buffer
            cbInstruction* Instruction = malloc(sizeof(cbInstruction));
            
            // Check against all first 19 ops (they are functions calls)
            for(int i = 0; i < cbOpsFuncCount; i++)
            {
                // Match?
                if(strncmp(Token, cbOpsNames[i], TokenLength) == 0)
                {
                    Instruction->Op = (cbOps)i;
                    break;
                }
            }
            
            cbList_PushBack(InstructionsList, (void*)Instruction);
        }
        
        /*** Literals ***/
        
        // Reads all variable types
        else if(cbLang_IsInteger(Token, TokenLength) || cbLang_IsString(Token, TokenLength) || cbLang_IsFloat(Token, TokenLength) || cbLang_IsBoolean(Token, TokenLength))
            Error = cbLang_LoadLiteral(InstructionsList, DataList, Token, TokenLength);
        
        /*** Variables ***/
        
        // If variable
        else if(cbLang_IsVariable(Token, TokenLength))
            Error = cbLang_LoadVariable(InstructionsList, VariablesList, Token, TokenLength);
        
        /*** Unknown Token ***/
        
        // Post error
        else
            Error = cbError_UnknownToken;
        
        // Done with token, release it
        free(Token);
    }
    
    // Release all remaining tokens
    while((Token = cbList_PopFront(&OpsList)) != NULL)
        free(Token);
    
    return Error;
}

cbError cbParse_ParseExpression(const char* Expression, cbList* OutputBuffer)
{
    // Ignore if null
    if(Expression == NULL)
        return cbError_Null;
    
    // Default to no error
    cbError Error = cbError_None;
    
    // Define the output queue and operator stack
    cbList Output, Operators;
    cbList_Init(&Output);
    cbList_Init(&Operators);
    
    // For each token in this expression
    const char* Token = Expression;
    size_t TokenLength = 0;
    while((Token = cbParse_GetToken(Token + TokenLength, &TokenLength)) != NULL && TokenLength > 0)
    {
        /*** Apply Shunting-Yard Algorithm ***/
        
        // Allocate string in the heap
        char* TokenString = malloc(TokenLength + 1);
        strncpy(TokenString, Token, TokenLength);
        TokenString[TokenLength] = 0;
        
        // If number, string, or variable, add to output queue
        if(cbLang_IsInteger(TokenString, TokenLength) || cbLang_IsString(TokenString, TokenLength) || cbLang_IsVariable(TokenString, TokenLength))
        {
            cbList_PushBack(&Output, TokenString);
        }
        // If function, add to op stack
        else if(cbLang_IsFunction(TokenString, TokenLength))
        {
            cbList_PushFront(&Operators, TokenString);
        }
        // If function argument separator
        else if(TokenLength == 1 && TokenString[0] == ',')
        {
            // Keep popping off operators onto the output stack until we see a left paren
            char* TopOp = NULL;
            while(true)
            {
                // Pop off the operators list
                TopOp = cbList_PeekFront(&Operators);
                
                // If left parenth, stop looping
                if(TopOp != NULL && strlen(TopOp) == 1 && TopOp[0] == '(')
                {
                    break;
                }
                else if(TopOp == NULL)
                {
                    Error = cbError_ParenthMismatch;
                    break;
                }
                else
                {
                    TopOp = cbList_PopFront(&Operators);
                    cbList_PushBack(&Output, TopOp);
                }
            }
        }
        // If operator..
        else if(cbLang_IsOp(TokenString, TokenLength))
        {
            // While there is an op on the operators stack
            char* TopOp = NULL;
            while((TopOp = cbList_PeekFront(&Operators)) != NULL && cbLang_IsOp(TopOp, strlen(TopOp)))
            {
                // Get string length to help in comparison
                size_t TopOpLength = strlen(TopOp);
                
                // If the precedence of 01 is less than 02
                if((cbLang_OpLeftAssoc(TokenString, TokenLength) && cbLang_OpLeftAssoc(TokenString, TokenLength) <= cbLang_OpLeftAssoc(TopOp, TopOpLength)) || (!cbLang_OpLeftAssoc(TokenString, TokenLength) && cbLang_OpLeftAssoc(TokenString, TokenLength) < cbLang_OpLeftAssoc(TopOp, TopOpLength)))
                {
                    // Pop off operators stack and push it to the end of the output
                    cbList_PopFront(&Operators);
                    cbList_PushBack(&Output, TopOp);
                }
                else
                    break;
            }
            
            // Add to the top of the ops queue
            cbList_PushFront(&Operators, TokenString);
        }
        // If opening parenth
        else if(TokenLength == 1 && TokenString[0] == '(')
        {
            cbList_PushFront(&Operators, TokenString);
        }
        // If closing parent
        else if(TokenLength == 1 && TokenString[0] == ')')
        {
            // Keep popping off operators into the output queue
            // until we run into the opening parenth
            char* TopOp = NULL;
            while(true)
            {
                // Grab the op
                TopOp = cbList_PopFront(&Operators);
                
                // If opening parenth, stop
                if(TopOp != NULL && TopOp[0] == '(')
                {
                    free(TopOp);
                    break;
                }
                // Else, push these ops onto the output
                else if(TopOp == NULL)
                {
                    Error = cbError_ParenthMismatch;
                    break;
                }
                else
                    cbList_PushBack(&Output, TopOp);
            }
            
            // Special rule: if the top of the stack is a function, pop off and push into output
            TopOp = cbList_PeekFront(&Operators);
            if(TopOp != NULL && cbLang_IsFunction(TopOp, strlen(TopOp)))
            {
                cbList_PopFront(&Operators);
                cbList_PushBack(&Output, TopOp);
            }
        }
        
        // Else, fail out
        else
        {
            // Never able to user string, release it
            free(TokenString);
            
            // Return error
            Error = cbError_UnknownToken;
            break;
        }
    }
    
    // No more tokens on this line, pop all ops back into the output
    char* TopOp = NULL;
    while((TopOp = cbList_PopFront(&Operators)) != NULL)
        cbList_PushBack(&Output, TopOp);
    
    // Post the buffer we generated
    *OutputBuffer = Output;
    
    // All done
    return Error;
}

cbError cbParse_ParseFor(const char* Expression, char** IteratorExp, char** MinExp, char** MaxExp, char** IncrementExp)
{
    // Default all out-strings as null, so if we fail (i.e. missing args)
    // we know which were allocated and which were not
    *IteratorExp = *MinExp = *MaxExp = *IncrementExp = NULL;
    
    // Find the first token (starts after the first parenth)
    size_t Start, End;
    size_t LineLength = strlen(Expression);
    
    for(Start = 0; Start < LineLength; Start++)
        if(Expression[Start] == '(')
            break;
    Start++;
    
    // Fail out if start is too far
    if(Start >= LineLength)
        return cbError_ParenthMismatch;
    
    // Find the end (but unlike Start, we stay here)
    for(End = LineLength - 1; End > Start; End--)
        if(Expression[End] == ')')
            break;
    
    // If the end colides with start, give up
    if(End <= Start)
        return cbError_ParenthMismatch;
    
    // For each of the four sub-strings find them using the comma
    int ArgCount = 0;
    for(size_t i = Start; i <= End; i++)
    {
        // If we ever encounter a comma, save the string before it
        if(Expression[i] == ',' || i == End)
        {
            // Remove front whitespace
            while(isspace(Expression[Start]) && Start < End)
                Start++;
            
            // Allocate, copy, and cap
            int TempLength = (int)i - (int)Start;
            if(TempLength <= 0)
                return cbError_MissingArgs;
            
            char* TempBuffer = malloc(TempLength + 1);
            strncpy(TempBuffer, Expression + Start, TempLength);
            TempBuffer[TempLength] = 0;
            
            // Remove back whitespace
            for(size_t j = TempLength - 1; j > 0; j--)
            {
                if(isspace(TempBuffer[j]))
                {
                    TempBuffer[j] = 0;
                    break;
                }
            }
            
            // Post in the correct arg index
            if(ArgCount == 0)
                *IteratorExp = TempBuffer;
            else if(ArgCount == 1)
                *MinExp = TempBuffer;
            else if(ArgCount == 2)
                *MaxExp = TempBuffer;
            else if(ArgCount == 3)
                *IncrementExp = TempBuffer;
            
            // Set start to copy the position after this string
            Start = i + 1;
            
            // Grow count
            ArgCount++;
        }
    }
    
    // We should always have just four arguments, fail out otherwise
    if(ArgCount != 4)
        return cbError_MissingArgs;
    
    // First arg must be a variable
    if(!cbLang_IsVariable(*IteratorExp, strlen(*IteratorExp)))
        return cbError_TypeMismatch;
    
    // No error
    return cbError_None;
}

const char* cbParse_GetToken(const char* String, size_t* TokenLength)
{
    // Start and end
    size_t Start, End;
    size_t StringLength = strlen(String);
    
    // Keep skipping white spaces
    for(Start = 0; Start < StringLength; Start++)
        if(!isspace(String[Start]))
            break;
    
    // If this is a string literal (must be in quotes), seek until next quote
    if(String[Start] == '"')
    {
        // Keep searching until the end
        for(End = Start + 1; End < StringLength; End++)
        {
            if(String[End] == '"')
            {
                // Grow to include last quote
                End++;
                break;
            }
        }
    }
    // Else if we hit a math operator
    else if(String[Start] == '(' || String[Start] == ')' || String[Start] == '+' || String[Start] == '-' ||
            String[Start] == '*' || String[Start] == '/' || String[Start] == '%' || String[Start] == '=')
    {
        End = Start + 1;
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
    // Else, arg. separator? (i.e. a comma)
    else if((StringLength - Start) >= 1 && String[0] == ',')
    {
        End = Start + 1;
    }
    // Else, just keep skipping to any sort of non-alphanum character
    else
    {
        // If white space or a special single-char, just stop
        for(End = Start; End < StringLength; End++)
            if(isspace(String[End]) || !isalnum(String[End]))
                break;
    }
    
    // Return length and string
    *TokenLength = End - Start;
    return String + Start;
}

cbError cbLang_LoadVariable(cbList* InstructionsList, cbList* VariablesList, char* Token, size_t TokenLength)
{
    // Does this variable name exist?
    int Offset = cbList_FindOffset(VariablesList, Token, cbList_ComparePointer);
    
    // If the variable does not exist, add to the list to get the offset
    if(Offset < 0)
    {
        // Make a copy on the heap
        char* TokenCopy = malloc(TokenLength + 1);
        strncpy(TokenCopy, Token, TokenLength + 1);
        
        Offset = (int)cbList_GetCount(VariablesList);
        cbList_PushBack(VariablesList, TokenCopy);
    }
    
    // Load the variable from the stack base onto the top of the stack
    cbInstruction* Instruction = malloc(sizeof(cbInstruction));
    
    Instruction->Op = cbOps_LoadVar;
    Instruction->Arg = -(Offset + 1) * sizeof(cbVariable); // + 1 because the data ends at the stack base address
    
    cbList_PushBack(InstructionsList, (void*)Instruction);
    
    // No problem
    return cbError_None;
}

cbError cbLang_LoadLiteral(cbList* InstructionsList, cbList* DataList, char* Token, size_t TokenLength)
{
    // Create variable and the variable offset
    cbVariable* Var = malloc(sizeof(cbVariable));
    
    // Convert to integer
    if(cbLang_IsInteger(Token, TokenLength))
    {
        int Integer;
        if(sscanf(Token, "%d", &Integer) <= 0)
            return cbError_ParseInt;
        
        Var->Type = cbType_Int;
        Var->Data.Int = Integer;
    }
    // Convert to string
    else if(cbLang_IsString(Token, TokenLength))
    {
        // Alloc, copy, cap
        Var->Type = cbType_String;
        Var->Data.String = malloc(TokenLength + 1);
        strncpy(Var->Data.String, Token, TokenLength);
        Var->Data.String[TokenLength] = 0;
    }
    //Convert to float
    else if(cbLang_IsFloat(Token, TokenLength))
    {
        float Float;
        if(sscanf(Token, "%f", &Float) <= 0)
            return cbError_ParseFloat;
        
        Var->Type = cbType_Float;
        Var->Data.Float = Float;
    }
    //Convert to boolean
    else if(cbLang_IsFloat(Token, TokenLength))
    {
        bool Bool = false;
        if(strcmp(Token, "true") == 0)
            Bool = true;
        else if(strcmp(Token, "false") == 0)
            Bool = false;
        else
            return cbError_TypeMismatch;
        
        Var->Type = cbType_Bool;
        Var->Data.Bool = Bool;
    }
    else
        return cbError_TypeMismatch;
    
    // Register this data in the static data list and keep a handle
    cbList_PushBack(DataList, Var);
    int AddressIndex = (int)cbList_GetCount(DataList) - 1;
    
    // Create load data instruction
    cbInstruction* Instruction = malloc(sizeof(cbInstruction));
    Instruction->Op = cbOps_LoadData;
    Instruction->Arg = AddressIndex * (int)sizeof(cbVariable); // Turn index-offset into byte offset
    
    // Save load instruction
    cbList_PushBack(InstructionsList, (void*)Instruction);
    
    // No error
    return cbError_None;
}

cbError cbLang_LoadOp(cbList* InstructionsList, char* Token, size_t TokenLength)
{
    // Add instruction into buffer
    cbInstruction* Instruction = malloc(sizeof(cbInstruction));
    Instruction->Arg = 0;
    
    // Tripple-char
    if(TokenLength == 3 && strncmp(Token, "and", 3) == 0)
        Instruction->Op = cbOps_And;
    else if(TokenLength == 2 && strncmp(Token, "or", 2) == 0)
        Instruction->Op = cbOps_Or;
    else if(TokenLength == 1 && strncmp(Token, "!", 2) == 0)
        Instruction->Op = cbOps_Not;
    
    // Double-char
    else if(Token[0] == '=' && Token[1] == '=')
        Instruction->Op = cbOps_Eq;
    else if(Token[0] == '!' && Token[1] == '=')
        Instruction->Op = cbOps_NotEq;
    else if(Token[0] == '>' && Token[1] == '=')
        Instruction->Op = cbOps_GreaterEq;
    else if(Token[0] == '<' && Token[1] == '=')
        Instruction->Op = cbOps_LessEq;
    
    // Single-char
    else if(Token[0] == '+')
        Instruction->Op = cbOps_Add;
    else if(Token[0] == '-')
        Instruction->Op = cbOps_Sub;
    else if(Token[0] == '*')
        Instruction->Op = cbOps_Mul;
    else if(Token[0] == '/')
        Instruction->Op = cbOps_Div;
    else if(Token[0] == '%')
        Instruction->Op = cbOps_Mod;
    else if(Token[0] == '=')
        Instruction->Op = cbOps_Set;
    else if(Token[0] == '<')
        Instruction->Op = cbOps_Less;
    else if(Token[0] == '>')
        Instruction->Op = cbOps_Greater;
    else
        return cbError_UnknownOp;
    
    cbList_PushBack(InstructionsList, (void*)Instruction);
    
    return cbError_None;
}
