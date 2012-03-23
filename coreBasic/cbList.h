/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: cbList.h/c
 Desc: Defines and implements a generic container based on a
 doubly-linked list. Acts both as a queue and a stack, where you
 can pop off on either ends, or add on either ends.
 
 Warning: the list can ONLY handle references to objects. Since
 C does not support templating (seen in C++), it is up the end-
 developer to deal with the memory management at their own
 discretion. This generic list code only retains pointers and
 nothing else.
 
***************************************************************/

#ifndef __CBLIST_H__
#define __CBLIST_H__

#include <stdlib.h>

#ifndef _WIN32
    #include <stdbool.h>
#endif

// List node structure
typedef struct __cbListNode
{
    // Done long-form because of the type self-reference
    struct __cbListNode* Next;
    struct __cbListNode* Prev;
    
    void* Data;
} cbListNode;

// List data structure
typedef struct __cbList
{
    cbListNode* Front;
    cbListNode* Back;
    
    size_t Count;
} cbList;

// Allocate and initialize a list
void cbList_Init(cbList* List);

// Release all the content of the list
void cbList_Release(cbList* List);

// Get the element count of a given list
size_t cbList_GetCount(cbList* List);

// Push to the front of the given list
void cbList_PushFront(cbList* List, void* Data);

// Push to the back of the given list
void cbList_PushBack(cbList* List, void* Data);

// Pop the front of the given list
void* cbList_PopFront(cbList* List);

// Pop the back of the given list
void* cbList_PopBack(cbList* List);

// Return the front element of the given list without removing it
void* cbList_PeekFront(cbList* List);

// Return the back element of the given list without removing it
void* cbList_PeekBack(cbList* List);

// Returns the number of elements from the start of the list until an elements data passes
// the given comparison test with the given variable data; returns a negative number upon failure
int cbList_FindOffset(cbList* List, void* Data, bool (*ComparisonFunc)(void* A, void* B));

#endif
