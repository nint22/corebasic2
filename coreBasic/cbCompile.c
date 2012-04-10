/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#include "cbCompile.h"

bool cbParse_CompileProgram(cbSymbolsTable* SymbolsTable, cbList* ErrorList, cbVirtualMachine* Process)
{
    /*** Translate Lex-Tree to ByteCode ***/
    
    // Initialize the symbls table for code generation
    cbList_Init(&SymbolsTable->InstructionsList);
    cbList_Init(&SymbolsTable->DataList);
    cbList_Init(&SymbolsTable->VariablesList);
    cbList_Init(&SymbolsTable->JumpTable);
    cbList_Init(&SymbolsTable->LabelTable);
    cbList_Init(&SymbolsTable->BlockStack);
    
    // Print the parse tree (i.e. for each line...)
    size_t LineCount = cbList_GetCount(&SymbolsTable->LexTree);
    for(int i = 0; i < LineCount; i++)
    {
        // If the tree is empty, ignore
        cbLexNode* LineNode = cbList_GetElement(&SymbolsTable->LexTree, i);
        if(LineNode == NULL)
            continue;
        
        // Push the line number before parsing each line
        size_t LineNumber = LineNode->LineNumber;
        cbParse_LoadInstruction(SymbolsTable, cbOps_Nop, ErrorList, (int)LineNumber);
        cbParse_BuildNode(SymbolsTable, cbList_GetElement(&SymbolsTable->LexTree, i), ErrorList);
    }
    
    // Fail if the block stack isn't empty
    if(cbList_GetCount(&SymbolsTable->BlockStack) > 0)
        cbUtil_RaiseError(ErrorList, cbError_BlockMismatch, 0);
    
    /*** Place ByteCode into Memory ***/
    
    // 1. Push a stack-init function if there are any variables on the stack:
    // Lower the stack pointer (i.e. grow the stack) for all local variables
    size_t VarCount = cbList_GetCount(&SymbolsTable->VariablesList);
    if(VarCount > 0)
    {
        cbInstruction* SpaceInstruction = malloc(sizeof(cbInstruction));
        SpaceInstruction->Op = cbOps_AddStack;
        SpaceInstruction->Arg = -(int)(VarCount * sizeof(cbVariable));
        cbList_PushFront(&SymbolsTable->InstructionsList, SpaceInstruction);
    }
    
    // 2. Explicitly add a "halt" at the end of the program so we know
    // when the program exits normally
    cbParse_LoadInstruction(SymbolsTable, cbOps_Halt, ErrorList, 0);
    
    // 3. Check: will we have enough space for all the instructions, variables, and strings?
    size_t InstrCount = cbList_GetCount(&SymbolsTable->InstructionsList);
    size_t DataCount = cbList_GetCount(&SymbolsTable->DataList);
    size_t TotalByteCount = sizeof(cbInstruction) * InstrCount + sizeof(cbVariable) + DataCount;
    
    // Add all string data from variables
    for(int i = 0; i < DataCount; i++)
    {
        cbVariable* var = cbList_GetElement(&SymbolsTable->DataList, i);
        if(var->Type == cbVariableType_String)
            TotalByteCount += strlen(var->Data.String) + 1;
    }
    
    // Are we small enough to continue compiling?
    if(TotalByteCount < Process->MemorySize)
    {
        // 4. Copy the code segment
        cbInstruction* Instr = NULL;
        InstrCount = 0;
        
        while((Instr = cbList_PopFront(&SymbolsTable->InstructionsList)) != NULL)
        {
            // Copy over data
            memcpy((cbInstruction*)Process->Memory + InstrCount, Instr, sizeof(cbInstruction));
            InstrCount++;
            
            // Release instruction
            free(Instr);
        }
        
        // 5. Above code (higher address), copy over static data (i.e. some literals and variables)
        cbVariable* Var = NULL;
        Process->DataVarCount = 0;
        Process->DataPointer = sizeof(cbInstruction) * InstrCount;
        VarCount = 0;
        
        while((Var = cbList_PopFront(&SymbolsTable->DataList)) != NULL)
        {
            // Copy over data
            memcpy((cbVariable*)((char*)Process->Memory + Process->DataPointer) + VarCount, Var, sizeof(cbVariable));
            VarCount++;
            
            // Release node
            free(Var);
            
            // Grow variable count
            Process->DataVarCount++;
        }
        
        // 6. For each data that is a string, copy the string itself to the end of the
        // data segment, thus turning this var into a reference to the string
        
        // The offset of where the strings should be stored
        size_t ByteOffset = Process->DataPointer + DataCount * sizeof(cbVariable);
        
        // For each variable already placed
        for(Var = (cbVariable*)((char*)Process->Memory + Process->DataPointer); Var < (cbVariable*)((char*)Process->Memory + Process->DataPointer + DataCount * sizeof(cbVariable)); Var++)
        {
            // Is this variable a string?
            if(Var->Type == cbVariableType_String)
            {
                // Grab from heap
                char* HeapString = Var->Data.String;
                
                // Have the variable point to the new address
                Var->Data.String = (char*)(ByteOffset - Process->DataPointer);
                
                // Copy the string with the null terminator
                size_t FullStringLength = strlen(HeapString) + 1;
                strncpy((char*)Process->Memory + ByteOffset, HeapString, FullStringLength);
                ByteOffset += FullStringLength;
                
                // Release from heap
                free(HeapString);
            }
        }
        
        // Save the heap-starting address
        Process->HeapPointer = ByteOffset;
        
        // 7. Match all goto's with labels
        while(cbList_GetCount(&SymbolsTable->JumpTable) > 0)
        {
            // Pop off the instruction we need to update and the label's name
            cbJump* Jump = cbList_PopFront(&SymbolsTable->JumpTable);
            
            // Find the label in the labels list
            int LabelObjIndex = cbList_FindOffset(&SymbolsTable->LabelTable, Jump->LabelName, cbList_CompareString);
            if(LabelObjIndex >= 0)
            {
                // Get the label info, the current instruction position, and set the jump offset
                cbLabel* LabelObj = cbList_GetElement(&SymbolsTable->LabelTable, LabelObjIndex);
                int InstrIndex = cbList_FindOffset(&SymbolsTable->InstructionsList, Jump->Instr, cbList_ComparePointer);
                
                // Offset = Label dest. - jump origin
                Jump->Instr->Arg = LabelObj->Index - InstrIndex;
            }
            // Else, never found, error
            else
                cbUtil_RaiseError(ErrorList, cbError_MissingLabel, Jump->LineNumber);
            
            // All done with this jump object
            free(Jump->LabelName);
            free(Jump);
        }
        
        // All done with compilation
    }
    else
        cbUtil_RaiseError(ErrorList, cbError_Overflow, 0);
    
    /*** Clean-Up ***/
    
    // For each lex-tree line, release
    while(cbList_GetCount(&SymbolsTable->LexTree) > 0)
    {
        cbLexNode* Node = cbList_PopFront(&SymbolsTable->LexTree);
        cbLex_DeleteNode(&Node);
    }
    cbList_Release(&SymbolsTable->LexTree);
    
    // Release symbols tables
    while(cbList_GetCount(&SymbolsTable->InstructionsList)) free(cbList_PopFront(&SymbolsTable->InstructionsList));
    cbList_Release(&SymbolsTable->InstructionsList);
    
    while(cbList_GetCount(&SymbolsTable->DataList)) free(cbList_PopFront(&SymbolsTable->DataList));
    cbList_Release(&SymbolsTable->DataList);
    
    while(cbList_GetCount(&SymbolsTable->VariablesList)) free(cbList_PopFront(&SymbolsTable->VariablesList));
    cbList_Release(&SymbolsTable->VariablesList);
    
    // Need to deep-free the jump tabel and the label table
    cbJump* Jump = NULL;
    while((Jump = cbList_PeekBack(&SymbolsTable->JumpTable)) != NULL)
    {
        free(Jump->LabelName);
        free(Jump);
    }
    cbList_Release(&SymbolsTable->JumpTable);
    
    // Nothing to release, the pointers point to the process memory map
    cbLabel* Label = NULL;
    while((Label = cbList_PeekBack(&SymbolsTable->LabelTable)) != NULL)
    {
        free(Label->LabelName);
        free(Label);
    }
    cbList_Release(&SymbolsTable->LabelTable);
    
    // Are there any errors?
    return cbList_GetCount(ErrorList) <= 0;
}

void cbParse_BuildNode(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList)
{
    // If null, give up
    if(Node == NULL)
        return;
    
    // Save the start of a while loop, so that we know the address of the conditional code
    // For while: saved so when we hit the matched "end" we jump to the start of the while's load code
    // For elif and else: we must save these locations, so that we can update the associated operator with the next address
    if(Node->Type == cbLexNodeType_Symbol && (Node->Data.Symbol == cbSymbol_StatementWhile || Node->Data.Symbol == cbSymbol_StatementElif || Node->Data.Symbol == cbSymbol_StatementElse))
    {
        cbJumpTarget* Target = malloc(sizeof(cbJumpTarget));
        Target->Symbol = Node->Data.Symbol;
        Target->Index = cbList_GetCount(&SymbolsTable->InstructionsList);
        Target->Instruction = NULL;
        cbList_PushBack(&SymbolsTable->BlockStack, Target);
    }
    
    // Seek left, right, then middle
    cbParse_BuildNode(SymbolsTable, Node->Left, ErrorList);
    cbParse_BuildNode(SymbolsTable, Node->Right, ErrorList);
    cbParse_BuildNode(SymbolsTable, Node->Middle, ErrorList);
    
    // Special statements / production rules
    if(Node->Type == cbLexNodeType_Symbol)
    {
        // The production rule types
        cbSymbol Symbol = Node->Data.Symbol;
        if(Symbol == cbSymbol_StatementIf)
        {
            // Push self onto block; used as a way to remember the index and to jump down on failure
            cbJumpTarget* Target = malloc(sizeof(cbJumpTarget));
            Target->Symbol = Node->Data.Symbol;
            Target->Index = cbList_GetCount(&SymbolsTable->InstructionsList);
            Target->Instruction = cbParse_LoadInstruction(SymbolsTable, cbOps_If, ErrorList, 0);
            cbList_PushBack(&SymbolsTable->BlockStack, Target);
        }
        else if(Symbol == cbSymbol_StatementElif)
        {
            // If the stack is empty, fail out
            if(cbList_GetCount(&SymbolsTable->BlockStack) <= 0)
                cbUtil_RaiseError(ErrorList, cbError_BlockMismatch, Node->LineNumber);
            else
            {
                // Get the current node and the prev. node
                cbJumpTarget* PrevTarget = cbList_PopBack(&SymbolsTable->BlockStack);
                cbJumpTarget* Self = cbList_PopBack(&SymbolsTable->BlockStack);
                Self->Index = cbList_GetCount(&SymbolsTable->InstructionsList);
                
                // If this prev target is an if, just update the jump vector to this loading overhead
                PrevTarget->Instruction->Arg = Self->Index - PrevTarget->Index;
                
                // Allocate and save the conditional op for this sub-block (as long as it isn't an else)
                if(Self->Symbol != cbSymbol_StatementElse)
                    Self->Instruction = cbParse_LoadInstruction(SymbolsTable, cbOps_If, ErrorList, 0);
                else
                    Self->Instruction = cbParse_LoadInstruction(SymbolsTable, cbOps_Goto, ErrorList, 0);
                
                // Push ourselves back into the list
                cbList_PushBack(&SymbolsTable->BlockStack, Self);
            }
        }
        else if(Symbol == cbSymbol_StatementGoto)
            cbParse_LoadGoto(SymbolsTable, Node, ErrorList);
        else if(Symbol == cbSymbol_StatementLabel)
            cbParse_LoadLabel(SymbolsTable, Node, ErrorList);
        else if(Symbol == cbSymbol_StatementWhile)
            ((cbJumpTarget*)cbList_PeekBack(&SymbolsTable->BlockStack))->Instruction = cbParse_LoadInstruction(SymbolsTable, cbOps_Goto, ErrorList, 0);
        else if(Symbol == cbSymbol_StatementFor)
            printf(" For is not yet implemented!\n");
        else if(Symbol == cbSymbol_End)
        {
            // If the stack is empty, fail out
            if(cbList_GetCount(&SymbolsTable->BlockStack) <= 0)
                cbUtil_RaiseError(ErrorList, cbError_BlockMismatch, Node->LineNumber);
            else
            {
                // Get the jump instruction we need to change and the current address index
                cbJumpTarget* Target = cbList_PopBack(&SymbolsTable->BlockStack);
                size_t OpCount = cbList_GetCount(&SymbolsTable->InstructionsList);
                
                // Set the jump location down to this location, but don't add any ops..
                Target->Instruction->Arg = OpCount - Target->Index - 1;
                
                // Done with target
                free(Target);
            }
        }
        // No else: there are production rules we don't care about like "expression"
    }
    // Terminals has data
    else if(Node->Type == cbLexNodeType_Terminal)
    {
        // What is the variable type?
        cbLexIDType Type = Node->Data.Terminal.Type;
        
        // Literal loaded from memory
        if(Type == cbLexIDType_Bool || Type == cbLexIDType_Float || Type == cbLexIDType_Int || Type == cbLexIDType_StringLit)
            cbParse_LoadLiteral(SymbolsTable, Node, ErrorList);
        
        // Variable that needs to be loaded dynamically
        else if(Type == cbLexIDType_Variable)
            cbParse_LoadVariable(SymbolsTable, Node, ErrorList);
        
        // Regular op turns straight to code *or* is a function call
        else if(Type == cbLexIDType_Op)
            cbParse_LoadInstruction(SymbolsTable, Node->Data.Terminal.Data.Op, ErrorList, 0);
        
        // Functions
        else if(Type == cbLexIDType_Func)
            cbParse_LoadFunction(SymbolsTable, Node, ErrorList);
        
        // Error
        else
            cbUtil_RaiseError(ErrorList, cbError_UnknownOp, Node->LineNumber);
    }
    // Unknown major type
    else
        cbUtil_RaiseError(ErrorList, cbError_UnknownToken, Node->LineNumber);
}

cbInstruction* cbParse_LoadInstruction(cbSymbolsTable* SymbolsTable, cbOps Op, cbList* ErrorList, int Arg)
{
    // Allocate and set
    cbInstruction* Instruction = malloc(sizeof(cbInstruction));
    Instruction->Op = Op;
    Instruction->Arg = Arg;
    
    // Push into the instructions list
    cbList_PushBack(&SymbolsTable->InstructionsList, Instruction);
    return Instruction;
}

void cbParse_LoadLiteral(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList)
{
    // Allocate variable and find type
    cbVariable* Var = malloc(sizeof(cbVariable));
    cbLexIDType Type = Node->Data.Terminal.Type;
    
    // Set variable type and data
    if(Type == cbLexIDType_Bool)
    {
        Var->Type = cbVariableType_Bool;
        Var->Data.Bool = Node->Data.Terminal.Data.Boolean;
    }
    else if(Type == cbLexIDType_Float)
    {
        Var->Type = cbVariableType_Float;
        Var->Data.Float = Node->Data.Terminal.Data.Float;
    }
    else if(Type == cbLexIDType_Int)
    {
        Var->Type = cbVariableType_Int;
        Var->Data.Int = Node->Data.Terminal.Data.Integer;
    }
    else if(Type == cbLexIDType_StringLit)
    {
        Var->Type = cbVariableType_String;
        Var->Data.String = cbUtil_stralloc(Node->Data.Terminal.Data.String); // Copy for now...
    }
    // Unknown...
    else
        cbUtil_RaiseError(ErrorList, cbError_UnknownToken, Node->LineNumber);
    
    // Register this data in the static data list and keep a handle
    int AddressIndex = (int)cbList_GetCount(&SymbolsTable->DataList);
    cbList_PushBack(&SymbolsTable->DataList, Var);
    
    // Create load data instruction
    cbParse_LoadInstruction(SymbolsTable, cbOps_LoadData, ErrorList, AddressIndex * (int)sizeof(cbVariable));
}

void cbParse_LoadVariable(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList)
{
    // Does this variable name exist?
    char* VariableName = Node->Data.Terminal.Data.String;
    int Offset = cbList_FindOffset(&SymbolsTable->VariablesList, VariableName, cbList_CompareString);
    
    // If the variable does not exist, add to the list to get the offset
    if(Offset < 0)
    {
        // Make a copy on the heap
        Offset = (int)cbList_GetCount(&SymbolsTable->VariablesList);
        cbList_PushBack(&SymbolsTable->VariablesList, cbUtil_stralloc(VariableName));
    }
    
    // Load the variable from the stack base onto the top of the stack
    // + 1 because the data ends at the stack base address
    cbParse_LoadInstruction(SymbolsTable, cbOps_LoadVar, ErrorList, -(Offset + 1) * sizeof(cbVariable));
}

void cbParse_LoadGoto(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList)
{
    // Create a jump object
    cbJump* Jump = malloc(sizeof(cbJump));
    Jump->Instr = cbParse_LoadInstruction(SymbolsTable, cbOps_Goto, ErrorList, 0);
    Jump->LabelName = cbUtil_stralloc(Node->Data.Terminal.Data.String);
    Jump->LineNumber = Node->LineNumber;
    
    // Save into the jump table
    cbList_PushBack(&SymbolsTable->JumpTable, Jump);
}

void cbParse_LoadLabel(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList)
{
    // Create a label object
    cbLabel* Label = malloc(sizeof(cbLabel));
    Label->Index = cbList_GetCount(&SymbolsTable->InstructionsList);
    Label->LabelName = cbUtil_stralloc(Node->Data.Terminal.Data.String);
    
    // Save into the labels table
    cbList_PushBack(&SymbolsTable->LabelTable, Label);
}

void cbParse_LoadFunction(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList)
{
    /*
      Built-in functions that natively run in the VM
       cbOps_Input,
       cbOps_Disp,
       cbOps_Output,
       cbOps_GetKey,
       cbOps_Clear,
    */
    
    // This node itself contains the function name, while the right points to the args list
    const char* FuncName = Node->Data.Terminal.Data.String;
    size_t ArgCount = cbLex_GetArgCount(Node->Right);
    
    // Hard-coded function check
    cbOps OpFunc = cbOps_Nop;
    if(strcmp(FuncName, "input") == 0 && ArgCount == 0)
        OpFunc = cbOps_Input;
    else if(strcmp(FuncName, "disp") == 0 && ArgCount == 1)
        OpFunc = cbOps_Disp;
    else if(strcmp(FuncName, "output") == 0 && ArgCount == 3)
        OpFunc = cbOps_Output;
    else if(strcmp(FuncName, "getkey") == 0 && ArgCount == 0)
        OpFunc = cbOps_GetKey;
    else if(strcmp(FuncName, "clear") == 0 && ArgCount == 0)
        OpFunc = cbOps_Clear;
    
    // Never matched, raise error
    if(OpFunc == cbOps_Nop)
        cbUtil_RaiseError(ErrorList, cbError_InvalidID, Node->LineNumber);
    else
        cbParse_LoadInstruction(SymbolsTable, OpFunc, ErrorList, 0);
}
