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
    
    /*** Parse & Compile Code ***/
    
    // Null out the processor
    memset((void*)Processor, 0, sizeof(cbVirtualMachine));
    
    // Parse the code
    cbParse_ParseProgram(Code, ErrorList);
    
    // If there are any errors, dont bother with the processor init
    if(cbList_GetCount(ErrorList) > 0)
        return false;
    
    /*** Initialize Machine ***/
    
    // Save standard i/o buffers immediately
    Processor->StreamOut = StreamOut;
    Processor->StreamIn = StreamIn;
    
    // Save screen size and allocate buffer
    Processor->ScreenWidth = ScreenWidth;
    Processor->ScreenHeight = ScreenHeight;
    Processor->ScreenBuffer = malloc(ScreenWidth * ScreenHeight);
    
    // TODO:
    // Initiailize correct machine registers
    
    // Done without any erorrs
    return true;
}

cbError cbInit_LoadByteCode(cbVirtualMachine* Processor, unsigned long MemorySize, FILE* InFile, FILE* StreamOut, FILE* StreamIn, size_t ScreenWidth, size_t ScreenHeight)
{
    // TODO: NEED TO UPDATE
    /*
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
    */
    // No error
    return cbError_None;
}

cbError cbInit_SaveByteCode(cbVirtualMachine* Processor, FILE* OutFile)
{
    // TODO: NEED TO UPDATE
    /*
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
    */
    // No error
    return cbError_None;
}

cbError cbRelease(cbVirtualMachine* Processor)
{
    // Ignore if null
    if(Processor == NULL)
        return cbError_Null;
    
    // Release the allocated processor memory and graphics map
    free(Processor->Memory);
    free(Processor->ScreenBuffer);
    
    // Null out the processor
    memset((void*)Processor, 0, sizeof(cbVirtualMachine));
    
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
