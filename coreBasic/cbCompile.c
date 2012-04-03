/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#include "cbCompile.h"

bool cbParse_CompileProgram(cbSymbolsTable* SymbolsTable, cbList* ErrorList, cbVirtualMachine* Process)
{
    // Print the parse tree (i.e. for each line...)
    while(cbList_GetCount(&SymbolsTable->LexTree) > 0)
    {
        printf("\nNew line:\n");
        cbParse_BuildNode(SymbolsTable, cbList_PopFront(&SymbolsTable->LexTree), ErrorList);
    }
    
    // Finalize code
    
    // Copy code to VM
    
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
    }
    // Terminals has data
    else if(Node->Type == cbLexNodeType_Terminal)
    {
        // What is the variable type?
        cbLexIDType Type = Node->Data.Terminal.Type;
        if(Type == cbLexIDType_Bool)
            printf(" Terminal: (Boolean) %d\n", (int)Node->Data.Terminal.Data.Boolean);
        else if(Type == cbLexIDType_Float)
            printf(" Terminal: (Float) %f\n", Node->Data.Terminal.Data.Float);
        else if(Type == cbLexIDType_Int)
            printf(" Terminal: (Integer) %d\n", (int)Node->Data.Terminal.Data.Integer);
        else if(Type == cbLexIDType_Variable)
            printf(" Terminal: (Variable) \"%s\"\n", Node->Data.Terminal.Data.String);
        else if(Type == cbLexIDType_StringLit)
            printf(" Terminal: (String Literal) \"%s\"\n", Node->Data.Terminal.Data.String);
        else if(Type == cbLexIDType_Op)
            printf(" Terminal: (Op.) %d\n", (int)Node->Data.Terminal.Data.Op);
        else
            printf(" TYPE ERROR!\n");
    }
}
