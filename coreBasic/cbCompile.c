/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#include "cbCompile.h"

bool cbParse_CompileProgram(cbSymbolsTable* SymbolsTable, cbList* ErrorList, cbVirtualMachine* Process)
{
    // Initialize the symbls table for code generation
    cbList_Init(&SymbolsTable->InstructionsList);
    cbList_Init(&SymbolsTable->DataList);
    cbList_Init(&SymbolsTable->VariablesList);
    cbList_Init(&SymbolsTable->JumpTable);
    cbList_Init(&SymbolsTable->LabelTable);
    
    // Print the parse tree (i.e. for each line...)
    size_t LineCount = cbList_GetCount(&SymbolsTable->LexTree);
    for(int i = 0; i < LineCount; i++)
        cbParse_BuildNode(SymbolsTable, cbList_GetElement(&SymbolsTable->LexTree, i), ErrorList);
    
    // Finalize code
    //cbParse_CompileByteCode(SymbolsTable, ErrorList, Process);
    
    // Copy code to VM
    
    // Release lex-tree
    
    // Release tables
    
    // Are there any errors?
    return cbList_GetCount(ErrorList) <= 0;
}

void cbParse_BuildNode(cbSymbolsTable* SymbolsTable, cbLexNode* Node, cbList* ErrorList)
{
    // If null, give up
    if(Node == NULL)
        return;
    
    // Seek left, right, then middle
    cbParse_BuildNode(SymbolsTable, Node->Left, ErrorList);
    cbParse_BuildNode(SymbolsTable, Node->Right, ErrorList);
    cbParse_BuildNode(SymbolsTable, Node->Middle, ErrorList);
    
    // Special statements / production rules
    if(Node->Type == cbLexNodeType_Symbol)
    {
        // The production rule type
        cbSymbol Symbol = Node->Data.Symbol;
        if(Symbol == cbSymbol_StatementIf)
            printf(" If\n");
        else if(Symbol == cbSymbol_StatementElif)
            printf(" Elif\n");
        else if(Symbol == cbSymbol_StatementGoto)
            printf(" Goto\n");
        else if(Symbol == cbSymbol_StatementLabel)
            printf(" Label\n");
        else if(Symbol == cbSymbol_StatementWhile)
            printf(" While\n");
        else if(Symbol == cbSymbol_StatementFor)
            printf(" For\n");
        else if(Symbol == cbSymbol_End)
            printf(" End\n");
        else
            cbParse_RaiseError(ErrorList, cbError_UnknownToken, Node->LineNumber);
    }
    // Terminals has data
    else if(Node->Type == cbLexNodeType_Terminal)
    {
        // What is the variable type?
        cbLexIDType Type = Node->Data.Terminal.Type;
        
        // Literal loaded from memory
        if(Type == cbLexIDType_Bool || Type == cbLexIDType_Float || Type == cbLexIDType_Int || Type == cbLexIDType_StringLit)
            cbList_PushBack(&SymbolsTable->DataList, Node);
        
        // Variable that needs to be loaded dynamically
        else if(Type == cbLexIDType_Variable)
            printf(" Terminal: (Variable) \"%s\"\n", Node->Data.Terminal.Data.String);
        
        // Regular op turns straight to code
        // Note: Args are modified later on once all items are merged together
        else if(Type == cbLexIDType_Op)
            cbParse_LoadInstruction(SymbolsTable, Node->Data.Terminal.Data.Op, ErrorList, 0);
        
        // Error
        else
            cbParse_RaiseError(ErrorList, cbError_UnknownToken, Node->LineNumber);
    }
}

void cbParse_LoadInstruction(cbSymbolsTable* SymbolsTable, cbOps Op, cbList* ErrorList, int Arg)
{
    // Allocate and set
    cbInstruction* Instruction = malloc(sizeof(cbInstruction));
    Instruction->Op = Op;
    Instruction->Arg = Arg;
    
    // Push into the instructions list
    cbList_PushBack(&SymbolsTable->InstructionsList, Instruction);
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
