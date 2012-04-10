/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: main.cpp
 Desc: Main application entry point.
 
***************************************************************/

#include <stdio.h>
#include "cbLang.h"
#include "cbProcess.h"

// Returns the number of bytes of the given file (note: will need +1
// for null-term if storing as a string); also note that the read-head
// will be reset to the start of the file
static size_t getFileLength(FILE* FileHandle)
{
    fseek(FileHandle, 0, SEEK_END);
    size_t SourceFileLength = ftell(FileHandle);
    fseek(FileHandle, 0, SEEK_SET);
    return SourceFileLength;
}

// Print the help / usage of this application
static void help()
{
    printf("Usage: cbasic [options] source file...\n"
           "Options:\n"
           "  None         Interprets the given source file, then executes code\n"
           "  -h           Prints this help message\n"
           "  -v           Verbose mode, printing the instructions and memory maps\n"
           "  -o <name>    Generates and stores byte-code into the given output file\n"
           "  -i <file>    Executes the given byte-code file\n");
}

// Main application entry point
int main (int argc, const char * argv[])
{
    /*** Parse User Inputs ***/
    
    // Default arguments passed by the user
    bool IsVerbose = false;
    const char* SourceFileName = NULL;
    const char* OutFileName = NULL;
    const char* InFileName = NULL;
    
    // Print header info.
    unsigned int Major, Minor;
    cbGetVersion(&Major, &Minor);
    printf("\ncoreBasic Version %d.%d (Console Interface)\n", Major, Minor);
    
    // For each argument
    for(int i = 1; i < argc; i++)
    {
        if(strcmp(argv[i], "-h") == 0)
        {
            help();
            return 0;
        }
        else if(strcmp(argv[i], "-v") == 0)
        {
            IsVerbose = true;
        }
        else if(strcmp(argv[i], "-o") == 0)
        {
            if(i + 1 < argc)
                OutFileName = argv[++i];
        }
        else if(strcmp(argv[i], "-i") == 0)
        {
            if(i + 1 < argc)
                InFileName = argv[++i];
        }
        else if(SourceFileName == NULL)
        {
            SourceFileName = argv[i];
        }
        else
        {
            // Error out
            printf("Unknown argument: \"%s\"\n", argv[i]);
            return -1;
        }
    }
    
    // If no source file or input file, error out
    if(SourceFileName == NULL && InFileName == NULL)
    {
        printf("A source code file or compiled file name must be given to execute!\n");
        return -1;
    }
    
    // Fail out as we can't load both source code and compiled code
    if(SourceFileName != NULL && InFileName != NULL)
    {
        printf("Cannot load both source code and compiled code at once!\n");
        return -1;
    }
    
    // Fail out since we can't compile no source code
    if(SourceFileName == NULL && OutFileName != NULL)
    {
        printf("Unable to compile code, as no source file was given.\n");
        return -1;
    }
    
    /*** Load or Write Code ***/
    
    // Simulator and error flag
    cbVirtualMachine Simulator;
    cbList Errors;
    
    // Attempt to open the source file (read-binary mode)
    if(SourceFileName != NULL)
    {
        // Attempt to load file
        FILE* SourceFile = fopen(SourceFileName, "rb");
        if(SourceFile == NULL)
        {
            printf("Unable to open the given source file \"%s\"\n", SourceFileName);
            return -1;
        }
        
        // Get code size
        size_t SourceFileLength = getFileLength(SourceFile);
        
        // Allocate enough space to copy, copy, then cap
        char* SourceCode = malloc(SourceFileLength + 1);
        fread(SourceCode, 1, SourceFileLength, SourceFile);
        SourceCode[SourceFileLength] = 0;
        
        // Interprete code
        cbInit_LoadSourceCode(&Simulator, 1024, SourceCode, stdout, stdin, 0, 0, &Errors);
        
        // Close file stream
        free(SourceCode);
        fclose(SourceFile);
    }
    
    // Check for error
    size_t ErrorCount = cbList_GetCount(&Errors);
    if(ErrorCount > 0)
    {
        printf("> Program failed to compile, %lu errors\n", ErrorCount);
        for(size_t i = 0; i < ErrorCount; i++)
        {
            cbParseError* Error = cbList_PopFront(&Errors);
            printf(">> %lu: %s\n", Error->LineNumber, cbDebug_GetErrorMsg(Error->ErrorCode));
            free(Error);
        }
        fflush(stdout); // Xcode isn't printing out before quitting
        return -1;
    }
    
    /*
     OLD FEATURE:
    // If the user wants to just write out to the file buffer
    if(OutFileName != NULL && SourceFileName != NULL)
    {
        // Attempt to open
        FILE* OutFile = fopen(OutFileName, "wb");
        if(OutFile == NULL)
        {
            printf("Unable to open \"%s\" to write to\n", OutFileName);
            cbRelease(&Simulator);
            return -1;
        }
        
        // Write to file in the special format
        Error = cbInit_SaveByteCode(&Simulator, OutFile);
        
        // Close file handle
        fclose(OutFile);
    }
    
    // Else, it has to be compiled code
    if(InFileName != NULL)
    {
        // Attempt to load file
        FILE* CompiledFile = fopen(InFileName, "rb");
        if(CompiledFile == NULL)
        {
            printf("Unable to open the given compiled file \"%s\"\n", InFileName);
            return -1;
        }
        
        // Interprete code
        Error = cbInit_LoadByteCode(&Simulator, 1024, CompiledFile, stdout, stdin, 0, 0);
        
        // Close file stream
        fclose(CompiledFile);
    }
    */
    
    /*** Simulation ***/
    
    // Print out some helpful details if verbose
    if(IsVerbose)
    {
        cbDebug_PrintInstructions(&Simulator, stdout);
        cbDebug_PrintMemory(&Simulator, stdout);
    }
    
    // Helper and simulation flags
    cbError Error = cbError_None;
    cbInterrupt InterruptState = cbInterrupt_None;
    
    // Simulate until done
    printf("> Program executing\n");
    while(Error == cbError_None)
    {
        // Step, catching any errors or interruptions
        Error = cbStep(&Simulator, &InterruptState);
        
        // If interrupted for input
        if(Error == cbError_None && InterruptState != cbInterrupt_None)
        {
            // Get user input string
            char input[256];
            do {
                scanf("%s", input);
            } while(InterruptState == cbInterrupt_Input && strlen(input) <= 0);
            
            // Remove the interrupt state, posting the result of the user input
            cbStep_ReleaseInterrupt(&Simulator, input);
        }
    }
    
    // Error state:
    if(Error != cbError_None && Error != cbError_Halted)
        printf("> Error %d, line %lu: \"%s\"\n", Error, cbDebug_GetLine(&Simulator), cbDebug_GetErrorMsg(Error));
    else
        printf("> Program terminated normally\n");
    
    // Print ticks if verbose
    if(IsVerbose)
        printf("> Total ticks: %lu\n", cbDebug_GetTicks(&Simulator));
    
    // Release
    cbRelease(&Simulator);
    return 0;
}
