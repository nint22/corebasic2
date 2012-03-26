/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 ***************************************************************/

#include "cbLang.h"

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
                int Bottom = cbList_FindOffset(&InstructionsList, Goto, cbList_ComparePointer);
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
