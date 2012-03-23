/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 ***************************************************************/

#include "cbLang.h"

/*** Internal Comparison Finders for cbList Code ***/

static bool __cbList_CompareInt(void* A, void* B)
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

static bool __cbList_CompareString(void* A, void* B)
{
    // Both are strings, simple direct string compare is needed
    if(strcmp((const char*)A, (const char*)B) == 0)
        return true;
    else
        return false;
}

static bool __cbList_ComparePointer(void* A, void* B)
{
    // Straight address comparison
    return (A == B);
}

/*** General Function Implementation ***/

bool cbInit_LoadSourceCode(cbVirtualMachine* Processor, unsigned long MemorySize, const char* Code, FILE* StreamOut, FILE* StreamIn, size_t ScreenWidth, size_t ScreenHeight, cbList* ErrorList)
{
    // Reset the error list
    cbList_Init(ErrorList);
    
    // Ignore if any arg is null
    if(Processor == NULL || Code == NULL || StreamOut == NULL || StreamIn == NULL)
    {
        cbParse_RaiseError(ErrorList, cbError_Null, -1);
        return false;
    }
    
    // Define an error flag we can set over time
    cbError Error = cbError_None;
    
    // Save standard i/o buffers immediately
    Processor->StreamOut = StreamOut;
    Processor->StreamIn = StreamIn;
    
    // Save screen size
    Processor->ScreenWidth = ScreenWidth;
    Processor->ScreenHeight = ScreenHeight;
    
    /*** Prepare Code ***/
    
    // Our own working buffer
    size_t CodeLength = strlen(Code);
    char* CodeBuffer = malloc(CodeLength + 1); // Includes null-terminator
    char* CodeBufferStart = CodeBuffer;
    
    // Copy the source code to be manipulated
    // Note: we go up to the length, to copy over the null-terminator
    for(size_t i = 0; i <= CodeLength; i++)
        CodeBuffer[i] = Code[i];
    
    /*** Remove all comments ***/
    
    // Comments start with "//" and end with new-lines
    // Just fill those segments with carriage-returns, i.e. '\r'
    bool IsFilling = false;
    for(size_t i = 0; i < CodeLength - 1; i++)
    {
        if(CodeBuffer[i] == '/' && CodeBuffer[i + 1]  == '/')
            IsFilling = true;
        else if(CodeBuffer[i] == '\n')
            IsFilling = false;
        
        if(IsFilling)
            CodeBuffer[i] = '\r';
    }
    
    /*** Parse & Compile ***/
    
    // Create a list to handle all instructions and constants (static data section)
    cbList InstructionsList, DataList, VariablesList, JumpTable, LabelTable;
    cbList_Init(&InstructionsList); // Array of cbInstructions
    cbList_Init(&DataList);         // Array of cbVariables (static data)
    cbList_Init(&VariablesList);    // Array of cbVariables (offsets)
    cbList_Init(&JumpTable);        // Mixed-array of [0: cbInstructions (jumps), 1: label-names] (but not yet linked)
    cbList_Init(&LabelTable);       // Array of of __cbLabel-type labels (and their indices)
    
    // Parse the initial block
    Error = cbParse_ParseBlock(&CodeBuffer, &InstructionsList, &DataList, &VariablesList, &JumpTable, &LabelTable, 0);
    
    /*** Build Memory for Simulation ***/
    
    // Processor not halted
    Processor->Halted = false;
    Processor->InterruptState = cbInterrupt_None;
    
    // Prepare the global memory map of the simulator
    Processor->Memory = malloc(MemorySize);
    Processor->MemorySize = MemorySize;
    
    // Default to no tick count and line 1
    Processor->Ticks = 0;
    Processor->LineIndex = 1;
    
    // 1. Connect all labels and goto's
    
    // For each goto-label command
    cbInstruction* Goto = NULL;
    while((Goto = cbList_PopFront(&JumpTable)) != NULL)
    {
        // The args are actually malloc'ed strings, to use to find the label
        char* LabelName = cbList_PopFront(&JumpTable);
        
        // Attempt to find the correct label
        size_t LabelCount = cbList_GetCount(&LabelTable);
        bool Found = false;
        for(size_t i = 0; i < LabelCount && !Found; i++)
        {
            // Look at front
            __cbLabel* Label = cbList_PopFront(&LabelTable);
            
            // If found, set offset and flag as done
            if(strcmp(Label->Label, LabelName) == 0)
            {
                Found = true;
                int Top = Label->Index;
                int Bottom = cbList_FindOffset(&InstructionsList, Goto, __cbList_ComparePointer);
                Goto->Arg = (Top - Bottom + 1) * sizeof(cbInstruction);
            }
            
            // Put back for other jumps to test against
            cbList_PushBack(&LabelTable, Label);
        }
        
        // If never found, it's an error
        if(!Found)
            Error = cbError_MissingLabel;
        
        // Done
        free(LabelName);
    }
    
    // Release all label objects
    __cbLabel* Label = NULL;
    while((Label = cbList_PopBack(&LabelTable)) != NULL)
        free(Label);
    
    // 2. Add space for the variables and a stop command at the end of this block
    
    // Initialize stack pointers to the highest address (stack size set to 0)
    Processor->StackBasePointer = Processor->MemorySize;
    Processor->StackPointer = Processor->MemorySize;
    
    // Lower the stack pointer (i.e. grow the stack) for all local variables
    if(cbList_GetCount(&VariablesList) > 0)
    {
        cbInstruction* SpaceInstruction = malloc(sizeof(cbInstruction));
        SpaceInstruction->Op = cbOps_AddStack;
        SpaceInstruction->Arg = -(int)(cbList_GetCount(&VariablesList) * sizeof(cbVariable));
        cbList_PushFront(&InstructionsList, SpaceInstruction);
    }
    
    // 3. Add a standard halt instruction, so we know the program explicitly completed and didn't fail
    
    // Explicitly add a stop at the end of the instructions
    cbInstruction* StopInstruction = malloc(sizeof(cbInstruction));
    StopInstruction->Op = cbOps_Stop;
    StopInstruction->Arg = 0;
    cbList_PushBack(&InstructionsList, StopInstruction);
    
    // 4. Copy the code segment
    
    // Helper variables when walking through the lists
    void* Node = NULL;
    size_t ByteOffset = 0;
    
    // Copy code
    Processor->InstructionPointer = 0;
    while((Node = cbList_PopFront(&InstructionsList)) != NULL)
    {
        // Copy over data
        memcpy(Processor->Memory + ByteOffset, Node, sizeof(cbInstruction));
        ByteOffset += sizeof(cbInstruction);
        
        // Release node
        free(Node);
    }
    
    // 5. Above code, copy over static data
    
    // Default to no var count
    Processor->DataVarCount = 0;
    
    // Copy each variable
    Processor->DataPointer = ByteOffset;
    while((Node = cbList_PopFront(&DataList)) != NULL)
    {
        // Copy over data
        memcpy(Processor->Memory + ByteOffset, Node, sizeof(cbVariable));
        ByteOffset += sizeof(cbVariable);
        
        // Release node
        free(Node);
        
        // Grow variable count
        Processor->DataVarCount++;
    }
    
    // For each variable, copy string data as needed
    size_t OffsetEnd = ByteOffset;
    for(size_t Offset = Processor->DataPointer; Offset < OffsetEnd; Offset += sizeof(cbVariable))
    {
        // Is this variable a string?
        cbVariable* Variable = (cbVariable*)(Processor->Memory + Offset);
        if(Variable->Type == cbType_String)
        {
            // Grab from heap
            char* HeapString = Variable->Data.String;
            
            // Have the variable point to the new address
            Variable->Data.String = (char*)(ByteOffset - Processor->DataPointer);
            
            // Cap the string
            HeapString[strlen(HeapString) - 1] = 0;
            
            // Copy the string with the null terminator
            strncpy(Processor->Memory + ByteOffset, HeapString + 1, strlen(HeapString));
            ByteOffset += strlen(HeapString);
            
            // Release from heap
            free(HeapString);
        }
    }
    
    // 6. Save the heap-starting address
    Processor->HeapPointer = ByteOffset;
    
    // Done with code buffer
    free(CodeBufferStart);
    
    // Done with lists
    cbList_Release(&VariablesList);
    cbList_Release(&InstructionsList);
    cbList_Release(&DataList);
    cbList_Release(&JumpTable);
    cbList_Release(&LabelTable);
    
    // All done!
    return Error;
}

cbError cbInit_LoadByteCode(cbVirtualMachine* Processor, unsigned long MemorySize, FILE* InFile, FILE* StreamOut, FILE* StreamIn, size_t ScreenWidth, size_t ScreenHeight)
{
    // Ignore if any arg is null
    if(Processor == NULL || InFile == NULL || StreamOut == NULL || StreamIn == NULL)
        return cbError_Null;
    
    // Allocate the processor memory size and save streams
    Processor->Memory = malloc(MemorySize);
    Processor->MemorySize = MemorySize;
    Processor->StreamOut = StreamOut;
    Processor->StreamIn = StreamIn;
    
    // Save screen size
    Processor->ScreenWidth = ScreenWidth;
    Processor->ScreenHeight = ScreenHeight;
    
    // Initialize default pointers and vars
    Processor->StackBasePointer = Processor->StackPointer = MemorySize;
    Processor->InstructionPointer = 0;
    Processor->Halted = false;
    Processor->InterruptState = cbInterrupt_None;
    Processor->Ticks = 0;
    Processor->LineIndex = 1;
    
    // Copy the code and data pointer
    fread((void*)(&Processor->DataVarCount), sizeof(size_t), 1, InFile);
    fread((void*)(&Processor->DataPointer), sizeof(size_t), 1, InFile);
    fread((void*)(&Processor->HeapPointer), sizeof(size_t), 1, InFile);
    
    // Copy code segment to first segment
    fread((void*)Processor->Memory, Processor->DataPointer, 1, InFile);
    
    // Copy text (data) segment to second segment
    fread((void*)Processor->Memory + Processor->DataPointer, Processor->HeapPointer - Processor->DataPointer, 1, InFile);
    
    // No error
    return cbError_None;
}

cbError cbInit_SaveByteCode(cbVirtualMachine* Processor, FILE* OutFile)
{
    // Fail if either is null
    if(Processor == NULL || OutFile == NULL)
        return cbError_Null;
    
    // All we need to copy is the code and static data
    // The first size_t represents the end of the data segment
    // The second size_t represents the end of the static-data segment
    fwrite((void*)(&Processor->DataVarCount), sizeof(size_t), 1, OutFile);
    fwrite((void*)(&Processor->DataPointer), sizeof(size_t), 1, OutFile);
    fwrite((void*)(&Processor->HeapPointer), sizeof(size_t), 1, OutFile);
    
    // Dump the code segment
    fwrite(Processor->Memory, Processor->DataPointer, 1, OutFile);
    
    // Dump the static data segment
    fwrite(Processor->Memory + Processor->DataPointer, Processor->HeapPointer - Processor->DataPointer, 1, OutFile);
    
    // No error
    return cbError_None;
}

cbError cbRelease(cbVirtualMachine* Processor)
{
    // Ignore if null
    if(Processor == NULL)
        return cbError_Null;
    
    // Release the allocated processor memory
    free(Processor->Memory);
    Processor->Memory = NULL;
    Processor->MemorySize = 0;
    
    // Release the queued draw commands
    while(cbList_GetCount(&Processor->ScreenQueue) > 0)
        free(cbList_PopBack(&Processor->ScreenQueue));
    
    // Note: it is up to the user to release the given file streams
    return cbError_Null;
}

cbList cbHighlightCode(const char* Code)
{
    // Prepare a list to define nodes in
    cbList TokenColors;
    cbList_Init(&TokenColors);
    
    // For each line
    size_t CodeLength = strlen(Code);
    for(size_t i = 0; Code[i] != '\0' && i < CodeLength; )
    {
        // Keep moving ahead until we see a non-space character
        if(!isgraph(Code[i]))
        {
            i++;
            continue;
        }
        
        // Get the token at this position
        size_t TokenLength;
        cbParse_GetToken(Code + i, &TokenLength);
        
        // Is this valid?
        if(TokenLength > 0)
        {
            // Create a dummy token for now
            cbHighlightToken* Color = malloc(sizeof(cbHighlightToken));
            Color->Start = i;
            Color->Length = TokenLength;
            
            // Is this a comment?
            if(Code[i] == '/' && Code[i + 1] == '/')
            {
                Color->TokenType = cbTokenType_Comment;
                
                // Keep moving until we see the end-of-line
                while(Code[i] != '\n' && Code[i] != '\0')
                    i++;
                
                // This is the new correct length
                Color->Length += i - Color->Start - 1;
            }
            // Is it a keyword?
            else if(cbLang_IsReserved(Code + i, TokenLength))
                Color->TokenType = cbTokenType_Keyword;
            // Is it a string lit?
            else if(cbLang_IsString(Code + i, TokenLength))
                Color->TokenType = cbTokenType_StringLit;
            // Is it a constant?
            else if(cbLang_IsBoolean(Code + i, TokenLength) || cbLang_IsFloat(Code + i, TokenLength) || cbLang_IsInteger(Code + i, TokenLength))
                Color->TokenType = cbTokenType_NumericalLit;
            // Ignoring:
            /*
                cbTokenType_Variable,
                cbTokenType_Function,
            */
            // Else, undefined
            else
            {
                free(Color);
                Color = NULL;
            }
            
            // Add to tokens list
            if(Color != NULL)
                cbList_PushBack(&TokenColors, Color);
            
            // Move ahead for the next token
            i += TokenLength;
        }
        // Else, invalid token element, just move foward
        else
            i++;
    }
    
    // Return the list of highlights
    return TokenColors;
}

void cbDebug_PrintInstructions(cbVirtualMachine* Processor, FILE* OutHandle)
{
    // Print the header of the instruction stack
    unsigned int Major, Minor;
    cbGetVersion(&Major, &Minor);
    fprintf(OutHandle, "===  coreBasic(%d.%d) Instructions  ===\n", Major, Minor);
    
    // Get the instruction count
    size_t InstructionCount = Processor->DataPointer / sizeof(cbInstruction);
    fprintf(OutHandle, " Instruction Count: %lu\n", InstructionCount);
    fprintf(OutHandle, " Addr:    Op.   |    arg.  |\n\n");
    
    // For each instruction
    for(size_t InstructionIndex = 0; InstructionIndex < InstructionCount; InstructionIndex++)
    {
        // Get the instruction handle
        cbInstruction* Instruction = (cbInstruction*)Processor->Memory + InstructionIndex;
        
        // Right justify the string
        fprintf(OutHandle, " %04lu: ", InstructionIndex * sizeof(cbInstruction));
        const char* InstructionStr = cbOpsNames[Instruction->Op];
        for(int i = 0; i < 8 - strlen(InstructionStr); i++)
            fprintf(OutHandle, "%c", ' ');
        fprintf(OutHandle, "%s | %8d |\n", InstructionStr, Instruction->Arg);
    }
    
    fprintf(OutHandle, "\n");
}

void cbDebug_PrintMemory(cbVirtualMachine* Processor, FILE* OutHandle)
{
    // Print the header of the memory stack
    unsigned int Major, Minor;
    cbGetVersion(&Major, &Minor);
    fprintf(OutHandle, "=== coreBasic(%d.%d) Static Memory ===\n", Major, Minor);
    
    // Get the memory count
    size_t VariableCount = Processor->DataVarCount;
    fprintf(OutHandle, " Variable Count: %lu\n", VariableCount);
    fprintf(OutHandle, " Addr:  [Type    ]  Data\n\n");
    
    // For each variable
    for(size_t VariableIndex = 0; VariableIndex < VariableCount; VariableIndex++)
    {
        // Get the instruction handle
        cbVariable* Variable = (cbVariable*)(Processor->Memory + Processor->DataPointer) + VariableIndex;
        
        fprintf(OutHandle, " %04lu: ", VariableIndex * sizeof(cbVariable));
        
        if(Variable->Type == cbType_Int)
            fprintf(OutHandle, " [Integer ]  %d\n", Variable->Data.Int);
        else if(Variable->Type == cbType_Bool)
            fprintf(OutHandle, " [Bool    ]  %s\n", Variable->Data.Bool ? "true" : "false");
        else if(Variable->Type == cbType_Float)
            fprintf(OutHandle, " [Float   ]  %f\n", Variable->Data.Float);
        else if(Variable->Type == cbType_String)
            fprintf(OutHandle, " [String  ]  %lu: %s\n", (size_t)Variable->Data.String, (char*)(Processor->Memory + Processor->DataPointer + (size_t)Variable->Data.String));
        else if(Variable->Type == cbType_String)
            fprintf(OutHandle, " [Offset  ]  %d\n", Variable->Data.Offset);
    }
    
    // Print the rest of the data
    for(size_t DataIndex = VariableCount * sizeof(cbVariable); Processor->DataPointer + DataIndex < Processor->HeapPointer; DataIndex += sizeof(cbVariable))
    {
        // Retrieve single-byte data
        char* Data = Processor->Memory + Processor->DataPointer + DataIndex;
        
        // For each byte, write out the hex
        fprintf(OutHandle, " %04lu:  [Raw Data]  ", DataIndex);
        for(int i = 0; i < sizeof(cbVariable); i++)
            fprintf(OutHandle, "%02x ", *(Data + i));
        
        // For each byte, write out the ASCII
        fprintf(OutHandle, "  ");
        for(int i = 0; i < sizeof(cbVariable); i++)
            fprintf(OutHandle, "%c", isprint(*(Data + i)) ? *(Data + i) : '.');
        
        // End of this line
        fprintf(OutHandle, "\n");
    }
    
    fprintf(OutHandle, "\n");
}

size_t cbDebug_GetInstructionCount(cbVirtualMachine* Processor)
{
    return Processor->DataPointer / sizeof(cbInstruction);
}

size_t cbDebug_GetVariableCount(cbVirtualMachine* Processor)
{
    return Processor->DataVarCount;
}

size_t cbDebug_GetTicks(cbVirtualMachine* Processor)
{
    return Processor->Ticks;
}

size_t cbDebug_GetLine(cbVirtualMachine* Processor)
{
    return Processor->LineIndex;
}

const char* const cbDebug_GetOpName(cbOps Op)
{
    return cbOpsNames[Op];
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
            WhileOp->Arg = (int)(cbList_GetCount(InstructionsList) - cbList_FindOffset(InstructionsList, WhileOp, __cbList_ComparePointer)) * sizeof(cbInstruction);
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
            int ForIndex = cbList_FindOffset(InstructionsList, ActiveFor, __cbList_ComparePointer);
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
                int PrevConIndex = cbList_FindOffset(InstructionsList, PrevConditional, __cbList_ComparePointer);
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
                int PrevConIndex = cbList_FindOffset(InstructionsList, PrevConditional, __cbList_ComparePointer);
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
                int PrevConIndex = cbList_FindOffset(InstructionsList, PrevConditional, __cbList_ComparePointer);
                PrevConditional->Arg = ((int)cbList_GetCount(InstructionsList) - PrevConIndex) * sizeof(cbInstruction);
                PrevConditional = NULL;
            }
            	
            // Wrap up any dangling pointers to this position (end of conditionals)
            cbInstruction* JumpInstr = NULL;
            while((JumpInstr = cbList_PopBack(&ConditionalJumps)) != NULL)
            {
                // The address of this jump instruction
                int Top = cbList_FindOffset(InstructionsList, JumpInstr, __cbList_ComparePointer);
                
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
                if((cbOp_LeftAssoc(TokenString, TokenLength) && cbOp_LeftAssoc(TokenString, TokenLength) <= cbOp_LeftAssoc(TopOp, TopOpLength)) || (!cbOp_LeftAssoc(TokenString, TokenLength) && cbOp_LeftAssoc(TokenString, TokenLength) < cbOp_LeftAssoc(TopOp, TopOpLength)))
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
    int Offset = cbList_FindOffset(VariablesList, Token, __cbList_CompareString);
    
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
