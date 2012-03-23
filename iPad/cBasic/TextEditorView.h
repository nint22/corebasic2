/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: TextEditorView.h/m
 Desc: The text editor view itself; though not a controller of the
 rendering macanism, it does take care of higher-level user
 events.
 
***************************************************************/

#import <UIKit/UIKit.h>
#import "UIColorTextView.h"
#import "LineCountViewController.h"
#import "ContextMenu.h"

// Forced foward declare since we are making a reference to a parent
@class EditorViewController;

@interface TextEditorView : UIView <UITextViewDelegate, UIScrollViewDelegate, UITableViewDataSource, UITableViewDelegate>
{
    // The current keyboard size
    CGRect KeyboardRect;
    
    // The parent that owns this object
    EditorViewController* ParentController;
    
    // Undo and redo stacks
    // Undo: each action, push onto the stack, and if the stack is over 32, pop off the oldest version
    // Redo: everytime we undo, push onto redo; if there is a change made, clean redo stack
    NSMutableArray* UndoStack;
    NSMutableArray* RedoStack;
    
    // Our instructions table we will render
    UITableViewController* InstructionsTable;
    
    // Our line counter / breakpoint container
    LineCountViewController* LineColumn;
    
    // List of instruction strings
    NSArray* InstructionLines;
    
    // Menu view ontop of the text editor
    ContextMenu* EditorMenu;
}

// Initialize internal components after loading from nib
- (void)initialize: (UIViewController*) Parent;

// Attempt to undo (if possible)
- (bool)Undo;

// Attempt to redo (if possible)
- (bool)Redo;

// Set the internal text
- (void)setText:(NSString*)Text;

// Set the table for the compiled instructions based on a c-string (file contents)
- (void)SetInstructionsSource:(const char*)string;

// Regular text editor
@property (nonatomic, retain) IBOutlet UIColorTextView* TextField;

// Instructions table
@property (nonatomic, retain) IBOutlet UIView* LineColumnView;

// Instructions table
@property (nonatomic, retain) IBOutlet UIScrollView* InstructionsScrollView;

@end
