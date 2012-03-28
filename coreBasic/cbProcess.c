/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#include "cbProcess.h"

cbError cbStep(cbVirtualMachine* Processor, cbInterrupt* InterruptState)
{
    // Ignore if null
    if(Processor == NULL)
        return cbError_Null;
    
    // If proc. is halted, return halt state
    if(Processor->Halted)
        return cbError_Halted;
    
    // If proc. is interrupted, we are waiting for user input on stdin
    if(Processor->InterruptState != cbInterrupt_None)
        return cbError_None;
    
    // Error state
    cbError Error = cbError_None;
    
    // Get the instruction
    cbInstruction* Instruction = (cbInstruction*)((char*)Processor->Memory + Processor->InstructionPointer);
    
    // Execute the instruction
    switch(Instruction->Op)
    {
        // Conditional ops.
        case cbOps_If:
            Error = cbStep_If(Processor, Instruction);
            break;
        
        // Math ops.
        case cbOps_Add:
        case cbOps_Sub:
        case cbOps_Mul:
        case cbOps_Div:
        case cbOps_Mod:
            Error = cbStep_MathOp(Processor, Instruction);
            break;
        
        // Comparison algebra ops.
        case cbOps_Eq:
        case cbOps_NotEq:
        case cbOps_Greater:
        case cbOps_GreaterEq:
        case cbOps_Less:
        case cbOps_LessEq:
            Error = cbStep_CompOp(Processor, Instruction);
            break;
        
        // Boolean ops.
        case cbOps_Not:
        case cbOps_And:
        case cbOps_Or:
            Error = cbStep_LogicOp(Processor, Instruction);
            break;
        
        // Program control ops.
        case cbOps_Goto:
            Processor->InstructionPointer += Instruction->Arg - sizeof(cbInstruction);
            break;
        case cbOps_Stop:
            Processor->Halted = true;
            break;
        
        // Memory control
        case cbOps_Set:
            Error = cbStep_Store(Processor, Instruction);
            break;
        case cbOps_LoadData:
            // Stack grows, and copy over variable from data segment
            Processor->StackPointer -= sizeof(cbVariable);
            memcpy((char*)Processor->Memory + Processor->StackPointer, (char*)Processor->Memory + Processor->DataPointer + Instruction->Arg, sizeof(cbVariable));
            break;
        case cbOps_LoadVar:
            // Stack grows, and set variable to be a reference to the var from the base stack address
            Processor->StackPointer -= sizeof(cbVariable);
            ((cbVariable*)((char*)Processor->Memory + Processor->StackPointer))->Type = cbType_Offset;
            ((cbVariable*)((char*)Processor->Memory + Processor->StackPointer))->Data.Offset = Instruction->Arg;
            break;
        case cbOps_AddStack:
            Processor->StackPointer += Instruction->Arg;
            break;
        
        // Input control
        case cbOps_Pause:
            Processor->InterruptState = cbInterrupt_Pause;
            break;
        case cbOps_Input:
            Processor->InterruptState = cbInterrupt_Input;
            break;
        case cbOps_GetKey:
            Processor->InterruptState = cbInterrupt_GetKey;
            break;
            
        // Output control
        case cbOps_Disp:
            Error = cbStep_Disp(Processor, Instruction);
            break;
        case cbOps_Output:
            Error = cbStep_Output(Processor, Instruction);
            break;
        case cbOps_Clear:
            Error = cbStep_Clear(Processor, Instruction);
            break;
            
        // TODO
        case cbOps_Exec:
            break;
        case cbOps_Return:
            break;
            
        // Keywords that are not to become operators
        case cbOps_Else:
        case cbOps_End:
        case cbOps_Label:
        case cbOps_Func:
        case cbOps_Elif:
        case cbOps_While:
        case cbOps_For:
            printf("Warning: non-op detected!\n");
            break;
        
        // Nop: Does nothing except stalls a cycle and saves the current line number
        case cbOps_Nop:
            Processor->LineIndex = Instruction->Arg;
            break;
    }
    
    // If proc. was halted, just return, else tick
    if(Processor->Halted)
        return cbError_Halted;
    else
        Processor->Ticks++;
    
    // Grow instruction counter
    Processor->InstructionPointer += sizeof(cbInstruction);
    
    // Post interrupt (if any)
    *InterruptState = Processor->InterruptState;
    
    // Out of bounds instruction index check
    if(Processor->InstructionPointer >= Processor->DataPointer || Processor->StackPointer > Processor->StackBasePointer || Processor->StackPointer <= Processor->HeapPointer)
        Error = cbError_Overflow;
    
    // All done!
    return Error;
}

void cbStep_ReleaseInterrupt(cbVirtualMachine* Processor, const char* UserInput)
{
    // Clear interrupt
    cbInterrupt OldState = Processor->InterruptState;
    Processor->InterruptState = cbInterrupt_None;
    
    // If pause, just ignore
    if(OldState == cbInterrupt_Pause)
    {
        return;
    }
    // If get key, just return the first index, or 0, on the stack
    else if(OldState == cbInterrupt_GetKey)
    {
        // Turn into integer
        cbVariable UserVar;
        UserVar.Type = cbType_Int;
        UserVar.Data.Int = UserInput[0];
        
        Processor->StackPointer -= sizeof(cbVariable);
        memcpy(Processor->Memory + Processor->StackPointer, (void*)&UserVar, sizeof(cbVariable));
    }
    // If asking for full line
    else if(OldState == cbInterrupt_Input)
    {
        // Parse for each type
        cbVariable UserVar;
        
        // Line length
        size_t UserInputLength = strlen(UserInput);
        
        if(cbLang_IsInteger(UserInput, UserInputLength))
        {
            UserVar.Type = cbType_Int;
            sscanf(UserInput, "%d", &UserVar.Data.Int);
        }
        else if(cbLang_IsFloat(UserInput, UserInputLength))
        {
            UserVar.Type = cbType_Float;
            sscanf(UserInput, "%f", &UserVar.Data.Float);
        }
        else if(cbLang_IsBoolean(UserInput, UserInputLength))
        {
            UserVar.Type = cbType_Bool;
            if(UserInput[0] == 't')
                UserVar.Data.Bool = true;
            else
                UserVar.Data.Bool = false;
        }
        else // Not supported
        {
            // Post -1 [int]
            UserVar.Type = cbType_Int;
            UserVar.Data.Int = -1;
        }
        
        // Push variable into run-time memory stack
        Processor->StackPointer -= sizeof(cbVariable);
        memcpy(Processor->Memory + Processor->StackPointer, (void*)&UserVar, sizeof(cbVariable));
    }
}

const unsigned char* const cbStep_GetScreenBuffer(cbVirtualMachine* Processor)
{
    return Processor->ScreenBuffer;
}

cbError cbStep_MathOp(cbVirtualMachine* Processor, cbInstruction* Instruction)
{
    /*** Load Data ***/
    
    // Get both variables off the stack
    // Grow the stack pointer since we are essentially popping off A and replacing B with our result
    cbVariable* A = (cbVariable*)((char*)Processor->Memory + Processor->StackPointer);
    Processor->StackPointer += sizeof(cbVariable);
    cbVariable* B = (cbVariable*)((char*)Processor->Memory + Processor->StackPointer);
    
    // Our result should be on the stack, not the actual memory
    cbVariable* Out = B;
    
    // If A or B is an offset (i.e. pointer), correctly point to the variable itself, and not the reference
    if(A->Type == cbType_Offset)
        A = (cbVariable*)((char*)Processor->Memory + Processor->StackBasePointer + A->Data.Offset);
    if(B->Type == cbType_Offset)
        B = (cbVariable*)((char*)Processor->Memory + Processor->StackBasePointer + B->Data.Offset);
    
    // Only integers are supported at the moment
    if(A->Type != cbType_Int || B->Type != cbType_Int)
        return cbError_TypeMismatch;
    
    // The output, for now, will always be integers
    Out->Type = cbType_Int;
    
    /*** Apply Operands ***/
    
    // If addition
    if(Instruction->Op == cbOps_Add)
        Out->Data.Int = B->Data.Int + A->Data.Int;
    
    // If subtraction
    else if(Instruction->Op == cbOps_Sub)
        Out->Data.Int = B->Data.Int - A->Data.Int;
    
    // If multiplication
    else if(Instruction->Op == cbOps_Mul)
        Out->Data.Int = B->Data.Int * A->Data.Int;
    
    // If division
    else if(Instruction->Op == cbOps_Div)
    {
        if(A->Data.Int == 0)
            return cbError_DivZero;
        else
            Out->Data.Int = B->Data.Int / A->Data.Int;
    }
    
    // If mod
    else if(Instruction->Op == cbOps_Mod)
        Out->Data.Int = B->Data.Int % A->Data.Int;
    
    // No problem
    return cbError_None;
}

cbError cbStep_Store(cbVirtualMachine* Processor, cbInstruction* Instruction)
{
    // Get both variables off the stack; remove both completely
    cbVariable* A = (cbVariable*)((char*)Processor->Memory + Processor->StackPointer);
    Processor->StackPointer += sizeof(cbVariable);
    
    cbVariable* B = (cbVariable*)((char*)Processor->Memory + Processor->StackPointer);
    Processor->StackPointer += sizeof(cbVariable);
    
    // If A is an offset (i.e. pointer), correctly point to the variable itself, and not the reference
    if(A->Type == cbType_Offset)
        A = (cbVariable*)((char*)Processor->Memory + Processor->StackBasePointer + A->Data.Offset);
    
    // If B is not a variable, fail out
    if(B->Type != cbType_Offset)
        return cbError_ConstSet;
    // Else, good
    else
        B = (cbVariable*)((char*)Processor->Memory + Processor->StackBasePointer + B->Data.Offset);
    
    // Set data
    *B = *A;
    
    // No problem
    return cbError_None;
}

cbError cbStep_Disp(cbVirtualMachine* Processor, cbInstruction* Instruction)
{
    // Get variables off the stack; shrink as needed
    cbVariable* A = (cbVariable*)((char*)Processor->Memory + Processor->StackPointer);
    Processor->StackPointer += sizeof(cbVariable);
    
    // Dereference if needed
    if(A->Type == cbType_Offset)
        A = (cbVariable*)((char*)Processor->Memory + Processor->StackBasePointer + A->Data.Offset);
    
    if(A->Type == cbType_Int)
        fprintf(Processor->StreamOut, "%d", A->Data.Int);
    else if(A->Type == cbType_String)
    {
        // Pull out the string
        char* String = (char*)((char*)Processor->Memory + Processor->DataPointer + (size_t)A->Data.String);
        
        // For each character, print out, unless it is a system-character
        for(size_t i = 0; i < strlen(String); i++)
        {
            // Apply system-chars
            if(String[i] == '\\' && String[i + 1] == 'n')
            {
                i++;
                fprintf(Processor->StreamOut, "\n");
            }
            else
                fprintf(Processor->StreamOut, "%c", String[i]);
        }
    }
    else
        return cbError_TypeMismatch;
    
    // Flush out to the stream
    fflush(Processor->StreamOut);
    
    // No problem
    return cbError_None;
}

cbError cbStep_If(cbVirtualMachine* Processor, cbInstruction* Instruction)
{
    // Get variables off the stack; shrink as needed
    cbVariable* A = (cbVariable*)((char*)Processor->Memory + Processor->StackPointer);
    Processor->StackPointer += sizeof(cbVariable);
    
    // Is this variable false?
    if(A->Type != cbType_Int)
        return cbError_TypeMismatch;
    
    // Jump outside of conditional block to the instruction above target (at bottom of VM loop, we grow instruction)
    else if(A->Data.Int == 0)
        Processor->InstructionPointer += Instruction->Arg - sizeof(cbInstruction);
    
    // Else: Continiue execution
    
    // No problem
    return cbError_None;
}

cbError cbStep_CompOp(cbVirtualMachine* Processor, cbInstruction* Instruction)
{
    /*** Load Data ***/
    
    // Get both variables off the stack
    // Grow the stack pointer since we are essentially popping off A and replacing B with our result
    cbVariable* A = (cbVariable*)((char*)Processor->Memory + Processor->StackPointer);
    Processor->StackPointer += sizeof(cbVariable);
    cbVariable* B = (cbVariable*)((char*)Processor->Memory + Processor->StackPointer);
    
    // Our result should be on the stack, not the actual memory
    cbVariable* Out = B;
    
    // If A or B is an offset (i.e. pointer), correctly point to the variable itself, and not the reference
    if(A->Type == cbType_Offset)
        A = (cbVariable*)((char*)Processor->Memory + Processor->StackBasePointer + A->Data.Offset);
    if(B->Type == cbType_Offset)
        B = (cbVariable*)((char*)Processor->Memory + Processor->StackBasePointer + B->Data.Offset);
    
    // Only integers are supported at the moment
    if(A->Type != cbType_Int || B->Type != cbType_Int)
        return cbError_TypeMismatch;
    
    // The output, for now, will always be integers
    Out->Type = cbType_Int;
    
    /*** Apply Logic ***/
    
    switch (Instruction->Op)
    {
        case cbOps_Eq:
            Out->Data.Int = B->Data.Int == A->Data.Int;
            break;
        case cbOps_NotEq:
            Out->Data.Int = B->Data.Int != A->Data.Int;
            break;
        case cbOps_Greater:
            Out->Data.Int = B->Data.Int > A->Data.Int;
            break;
        case cbOps_GreaterEq:
            Out->Data.Int = B->Data.Int >= A->Data.Int;
            break;
        case cbOps_Less:
            Out->Data.Int = B->Data.Int < A->Data.Int;
            break;
        case cbOps_LessEq:
            Out->Data.Int = B->Data.Int <= A->Data.Int;
            break;
        default:
            return cbError_UnknownOp;
    }
    
    // No problem
    return cbError_None;
}

cbError cbStep_LogicOp(cbVirtualMachine* Processor, cbInstruction* Instruction)
{
    /*** Load Data ***/
    
    // Get a variable from the stack, or two if it is either the and, or ops.
    cbVariable* A = (cbVariable*)(Processor->Memory + Processor->StackPointer);
    Processor->StackPointer += sizeof(cbVariable);
    
    // If offset, not actual variable, point to the true variable
    if(A->Type == cbType_Offset)
        A = (cbVariable*)(Processor->Memory + Processor->StackBasePointer + A->Data.Offset);
    
    // If the not operator, apply it just on the one variable A
    if(Instruction->Op == cbOps_Not)
    {
        // Only support boolean or integers
        if(A->Type != cbType_Int && A->Type != cbType_Bool)
            return cbError_TypeMismatch;
        
        // Apply not-op
        A->Data.Int = !(A->Data.Int);
    }
    
    // Else, is is either and, or, thus needs variable B
    else
    {
        // Get the second variable out
        cbVariable* B = (cbVariable*)(Processor->Memory + Processor->StackPointer);
        if(B->Type == cbType_Offset)
            B = (cbVariable*)(Processor->Memory + Processor->StackBasePointer + B->Data.Offset);
        
        // Only support boolean or integers
        if(A->Type != cbType_Int && A->Type != cbType_Bool && B->Type != cbType_Int && B->Type != cbType_Bool)
            return cbError_TypeMismatch;
        
        // Apply 'and' op
        if(Instruction->Op == cbOps_And)
            B->Data.Int = A->Data.Int && B->Data.Int;
        // Apply 'or' op
        else if(Instruction->Op == cbOps_And)
            B->Data.Int = A->Data.Int || B->Data.Int;
    }
    
    // No problem
    return cbError_None;
}

cbError cbStep_Output(cbVirtualMachine* Processor, cbInstruction* Instruction)
{
    /*** Load Data ***/
    
    // Get color off
    cbVariable* C = (cbVariable*)(Processor->Memory + Processor->StackPointer);
    Processor->StackPointer += sizeof(cbVariable);
    
    // Get both positions off the stack
    cbVariable* Y = (cbVariable*)(Processor->Memory + Processor->StackPointer);
    Processor->StackPointer += sizeof(cbVariable);
    
    cbVariable* X = (cbVariable*)(Processor->Memory + Processor->StackPointer);
    Processor->StackPointer += sizeof(cbVariable);
    
    // If offset, not actual variable, point to the true variable
    if(X->Type == cbType_Offset)
        X = (cbVariable*)(Processor->Memory + Processor->StackBasePointer + X->Data.Offset);
    if(Y->Type == cbType_Offset)
        Y = (cbVariable*)(Processor->Memory + Processor->StackBasePointer + Y->Data.Offset);
    if(C->Type == cbType_Offset)
        C = (cbVariable*)(Processor->Memory + Processor->StackBasePointer + C->Data.Offset);
    
    /*** Render on-screen ***/
    
    // Type check
    if(X->Type != cbType_Int || Y->Type != cbType_Int || C->Type != cbType_Int)
        return cbError_TypeMismatch;
    
    // Bounds check
    if(X->Data.Int < 0 || X->Data.Int >= Processor->ScreenWidth || Y->Data.Int < 0 || Y->Data.Int >= Processor->ScreenHeight)
        return cbError_Overflow;
    
    // Find the byte position of the pixel
    Processor->ScreenBuffer[Processor->ScreenWidth * Y->Data.Int + X->Data.Int] = C->Data.Int;
    
    // No problem
    return cbError_None;
}

cbError cbStep_Clear(cbVirtualMachine* Processor, cbInstruction* Instruction)
{
    // Clear buffer
    memset((void*)Processor->ScreenBuffer, 0, Processor->ScreenWidth * Processor->ScreenHeight);
    
    // No error, ever
    return cbError_None;
}
