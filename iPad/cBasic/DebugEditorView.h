/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: DebugEditorView.h/m
 Desc: The three bottom debugging views combined into one
 centralized debugging view. A higher-level controller of
 events.
 
***************************************************************/

#import <UIKit/UIKit.h>
#import "ConsoleTextView.h"
#import "ScreenView.h"

// The target width of all text fields and cells
static const int DebugEditorView_Width = 500;

@interface DebugEditorView : UIView <UITextFieldDelegate, UIScrollViewDelegate, UITableViewDataSource, UITableViewDelegate>
{
    // Is full-screen'ed
    bool IsFullScreen;
    
    // List of memory strings
    NSArray* MemoryLines;
    
    // Our on-screen memory map
    UITableViewController* MemoryTable;
}

// The console text I/O system
@property (nonatomic, retain) IBOutlet ConsoleTextView* TextField;

// The screen itself that the program can write to
@property (nonatomic, retain) IBOutlet ScreenView* MainScreen;

// Our memory viewer
@property (nonatomic, retain) IBOutlet UIScrollView* MemoryScrollView;

// Initialize some GUI elements once the view is correctly loaded
- (void)Initialize;

// Set the table for the memory map based on a c-string (file contents)
- (void)SetDataSource:(const char*)string;

// Screen was pressed
- (void)ScreenPressed:(id)sender;

@end
