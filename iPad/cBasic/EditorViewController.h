/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: EditorViewController.h/m
 Desc: The main view controller for the overall IDE user interface.
 Here we manage the text editing, process controller, simulation,
 debugging, I/O, etc.
 
***************************************************************/

#import <UIKit/UIKit.h>
#import "TextEditorView.h"
#import "DebugEditorView.h"
#import "cbLang.h"
#import "cbProcess.h"
#import "PopDownController.h"

@interface EditorViewController : UIViewController <PopDownControllerDelegate>
{
    // Source file name
    NSString* FileName;
    NSString* ShortFileName;
    
    // Processor handle for simulation and any errors associated with it
    cbProcessor Processor;
    cbError SimulatorError;
    cbInterrupt InterruptState;
    
    // Main text editor and debug interface
    TextEditorView* ViewEditor;
    DebugEditorView* ViewDebug;
    
    // File stream I/O
    FILE* StreamOut;
    FILE* StreamIn;
    int StreamReadPos;
    
    // Date-time format used throughout the UI
    NSDateFormatter* DateTimeFormat;
    
    // Time when the simulation starts
    NSDate* SimulationStart;
    
    // Timer / loop used for the above mutex
    NSTimer* GUITimer;
    
    // A pop-over we use as the main menu of the application
    PopDownController* PopoverSelector;
    UIPopoverController* Popover;
}

// Editor-view buttons
@property (nonatomic, retain) IBOutlet UIButton* MenuButton;
@property (nonatomic, retain) IBOutlet UIButton* RunButton;
@property (nonatomic, retain) IBOutlet UIButton* StopButton;
@property (nonatomic, retain) IBOutlet UIButton* StepOverButton;
@property (nonatomic, retain) IBOutlet UIButton* StepIntoButton;
@property (nonatomic, retain) IBOutlet UIButton* StepOutButton;

// Add the text view controller
@property (nonatomic, retain) IBOutlet UIView* EditorView;
@property (nonatomic, retain) IBOutlet UIView* DebugView;

// Divider images
@property (nonatomic, retain) IBOutlet UIImageView* TopDiv;
@property (nonatomic, retain) IBOutlet UIImageView* BottomDiv;
@property (nonatomic, retain) IBOutlet UILabel* TopLabel;
@property (nonatomic, retain) IBOutlet UILabel* BottomLabel;

// Build information
@property (nonatomic, retain) IBOutlet UILabel* TopBuildLabel;
@property (nonatomic, retain) IBOutlet UILabel* BottomBuildLabel;
@property (nonatomic, retain) IBOutlet UIImageView* BuildIcon;

// View, because of the keyboard, has to change
-(void) keyboardWillShow:(CGRect) KeyboardSize;
-(void) keyboardWillHide:(CGRect) KeyboardSize;

// Animate an offset (i.e. when the keyboard comes up)
-(void) animateOffset:(int) KeyboardHeight;

// Run-button was pressed
-(IBAction)RunPressed:(id)sender;

// Menu was pressed
-(IBAction)MenuPressed:(id)sender;

// Stop pressed
-(IBAction)StopPressed:(id)sender;

// Load a source file
-(void) LoadFile:(NSString*) SourceFileName;

// Commit all changes to the file
-(void) SaveFile;

@end
