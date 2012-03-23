/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: cbProcess.h/c
 Desc: Defines and implements all simulation function in the
 given process.
 
***************************************************************/

#ifndef __CBPROCESS_H__
#define __CBPROCESS_H__

/*** Needed includes ***/

#include "cbUtil.h"
#include "cbTypes.h"

/*** Simulation Function ***/

// Step through a single instruction; returns a failure description enumeration
__cbEXPORT cbError cbStep(cbVirtualMachine* Processor, cbInterrupt* InterruptState);

// Release (set to false) the interrupt state; completing the input-interruption
__cbEXPORT void cbStep_ReleaseInterrupt(cbVirtualMachine* Processor, const char* UserInput);

// Returns true if the process has any queued drawing commands for the given screen buffer
// If there are no queued events, then false is retured. Note that this is a read-once function
// call where any following calls will result in any new outputs; i.e. you are reading from a queue
__cbEXPORT bool cbStep_GetOutput(cbVirtualMachine* Processor, unsigned int* x, unsigned int* y, unsigned int* color);

/*** Helper Functions ***/

// Takes two variables from the stack that have been pushed previously and post the result in said stack
// If there is an error (mismatched types), an error is returned, else no error is posted
cbError cbStep_MathOp(cbVirtualMachine* Processor, cbInstruction* Instruction);

// Takes the two variables from the stack (a and b, respect), and places A into B, assuming
// B is a variable, while A can either be a literal or variable. If B is not a variable, an error
// is returned
cbError cbStep_Store(cbVirtualMachine* Processor, cbInstruction* Instruction);

// Print the element on the stack, unless it is currently a variable reference (i.e. offset)
// Shrinks stack appropriatly appropriately
cbError cbStep_Disp(cbVirtualMachine* Processor, cbInstruction* Instruction);

// If the integer on the stack is 0 (false), then jump to the instructions arg, else (true),
// continue flow of execution without any interruption
cbError cbStep_If(cbVirtualMachine* Processor, cbInstruction* Instruction);

// Pulls off the two variables from the stack and does the proper comparison, pushing a
// true or false on the stack (as booleans)
cbError cbStep_CompOp(cbVirtualMachine* Processor, cbInstruction* Instruction);

// Apply boolean logical operators of not, and, or on the stack
cbError cbStep_LogicOp(cbVirtualMachine* Processor, cbInstruction* Instruction);

// Takes and pops off the three integers from the stack for position (tuple), and pixel color (range from 0 - 3)
cbError cbStep_Output(cbVirtualMachine* Processor, cbInstruction* Instruction);

// Clear out the output (of the screen, not file streams) to white
// This is done by placing a clear instruction (three MAX_UINT) integers into the screen buffer
cbError cbStep_Clear(cbVirtualMachine* Processor, cbInstruction* Instruction);

#endif
