/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: FileBrowserController.h/m
 Desc: The main file browser view controller. Takes care of
 high-level user events and manages custom rendering within
 the central scroll view.
 
***************************************************************/

#import <UIKit/UIKit.h>
#import "PopDownController.h"
#import "EditorViewController.h"

typedef enum __PopoverState_t
{
    PopoverState_NewFile,
    PopoverState_NewFolder,
    PopoverState_DeleteItem,
} PopoverState_t;

@interface FileBrowserController : UIViewController <PopDownControllerDelegate, UIAlertViewDelegate>
{
    // The active working directory
    NSString* DirName;
    
    // The popover that comes up when selecting a creation action
    PopDownController* PopoverSelector;
    
    // The active popover (only one at a time)
    UIPopoverController* Popover;
    
    // The file name text field
    UITextField* PopoverTextField;
    
    // List of current directory content
    NSArray* DirectoryContents;
    
    // Active state
    PopoverState_t PopoverState;
    
    // Document host viewer and back buttons
    UIViewController* WebViewController;
    UIButton* WebBackButton;
}

// Events to go back, help, add file/folder
- (IBAction)BackPressed:(id)Sender;
- (IBAction)AddFilePressed:(id)Sender;
- (IBAction)HelpPressed:(id)Sender;

// Back button (only active if not in root directory) and add button (only active if possible to add)
@property (nonatomic, retain) IBOutlet UIButton* BackButton;
@property (nonatomic, retain) IBOutlet UIButton* AddButton;

// Titles
@property (nonatomic, retain) IBOutlet UILabel* MajorLabel;
@property (nonatomic, retain) IBOutlet UILabel* MinorLabel;

// Scroll view
@property (nonatomic, retain) IBOutlet UIScrollView* ScrollView;

// Set the working directory of this file browser
- (void) setDirectory: (NSString*)DirectoryName;

// Load the saved directory and resets all necesary view components
- (void) loadDirectory;

// User selected item to create
-(void)itemSelected:(int)index;

// Returns the root navigation controller
- (UINavigationController*) getNavigationController;

@end
