/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#include "cbList.h"

void cbList_Init(cbList* List)
{
    List->Front = NULL;
    List->Back = NULL;
    List->Count = 0;
}

void cbList_Release(cbList* List)
{
    // For each node
    cbListNode* Node = List->Front;
    cbListNode* Prev = NULL;
    while(Node != NULL)
    {
        Prev = Node;
        Node = Node->Next;
        free(Prev);
    }
}

size_t cbList_GetCount(cbList* List)
{
    return List->Count;
}

void cbList_PushFront(cbList* List, void* Data)
{
    // Allocate new front node
    cbListNode* Front = (cbListNode*)malloc(sizeof(cbListNode));
    Front->Data = Data;
    
    // Define the next as null (since we are now the end of the list)
    // and the previous block as our next
    Front->Prev = NULL;
    Front->Next = List->Front;
    
    // Update the previous fronts's prev
    if(List->Front != NULL)
        List->Front->Prev = Front;
    List->Front = Front;
    
    // If the list is currently empty, make both front and back point to the same node
    if(List->Count <= 0)
        List->Back = Front;
    
    // Update the lists' Back
    List->Count++;
}

void cbList_PushBack(cbList* List, void* Data)
{
    // Allocate new front node
    cbListNode* Back = (cbListNode*)malloc(sizeof(cbListNode));
    Back->Data = Data;
    
    // Define the next as null (since we are now the end of the list)
    // and the previous block as our next
    Back->Prev = List->Back;
    Back->Next = NULL;
    
    // Update the previous end's next
    if(List->Back != NULL)
        List->Back->Next = Back;
    List->Back = Back;
    
    // If the list is currently empty, make both front and back point to the same node
    if(List->Count <= 0)
        List->Front = Back;
    
    // Update the lists' Back
    List->Count++;
}

void* cbList_PopFront(cbList* List)
{
    // Get front node
    cbListNode* Front = List->Front;
    
    // Access data if possible
    void* Data = NULL;
    if(Front != NULL)
    {
        // Get data
        Data = Front->Data;
        
        // Clip next node and change list's front
        if(Front->Next != NULL)
            Front->Next->Prev = NULL;
        List->Front = Front->Next;
        
        // Delete front if possible
        List->Count--;
        free(Front);
    }
    
    // If empty, null both
    if(List->Count <= 0)
        List->Front = List->Back = NULL;
    
    return Data;
}

void* cbList_PopBack(cbList* List)
{
    // Get back node
    cbListNode* Back = List->Back;
    
    // Access data if possible
    void* Data = NULL;
    if(Back != NULL)
    {
        // Get data
        Data = Back->Data;
        
        // Clip prev node and change list's back
        if(Back->Prev != NULL)
            Back->Prev->Next = NULL;
        List->Back = Back->Prev;
        
        // Delete front if possible
        List->Count--;
        free(Back);
    }
    
    // If empty, null both
    if(List->Count <= 0)
        List->Front = List->Back = NULL;
    
    return Data;
}

void* cbList_PeekFront(cbList* List)
{
    if(List == NULL)
        return NULL;
    else if(List->Front == NULL)
        return NULL;
    else
        return List->Front->Data;
}

void* cbList_PeekBack(cbList* List)
{
    if(List == NULL)
        return NULL;
    else if(List->Back == NULL)
        return NULL;
    else
        return List->Back->Data;
}

void* cbList_GetElement(cbList* List, int Index)
{
    // If the index is out of bounds, return null
    if(Index < 0 || Index >= List->Count)
        return NULL;
    
    // Seek through the list
    cbListNode* Node = List->Front;
    for(int i = 1; i <= Index; i++)
        Node = Node->Next;
    return Node->Data;
}

bool cbList_Subset(cbList* List, cbList* Subset, int Index, int n)
{
    // Bounds check
    if(Index < 0 || Index >= List->Count || Index + n > List->Count)
        return false;
    
    // Create a new list that is our subset
    cbList_Init(Subset);
    
    // Copy elements
    cbListNode* Node = List->Front;
    for(int i = 0; i < Index + n; i++)
    {
        // If we are in the range, copy over
        if(i >= Index)
            cbList_PushBack(Subset, Node->Data);
        
        // Next node
        Node = Node->Next;
    }
    
    // Done
    return true;
}

int cbList_FindOffset(cbList* List, void* Data, bool (*ComparisonFunc)(void* A, void* B))
{
    // Node count
    int NodeCount = 0;
    
    // For each node
    cbListNode* Node = List->Front;
    while(Node != NULL)
    {
        // Is this the node we want?
        if(ComparisonFunc(Data, Node->Data))
            break;
        
        // Not found, look at the next node
        Node = Node->Next;
        NodeCount++;
    }
    
    // All done (return -1 if we never found our node)
    return (Node == NULL) ? -1 : NodeCount;
}
