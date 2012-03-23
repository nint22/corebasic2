/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: PopDownController.h/m
 Desc: A trivial pop-down controller that allows the user to
 select any given menu options as defined in the "SetOptions:..."
 function.
 
***************************************************************/

#import <UIKit/UIKit.h>

// Callback routine
@protocol PopDownControllerDelegate
    @required
    -(void)itemSelected:(int)index;
@end

@interface PopDownController : UITableViewController
{
    id<PopDownControllerDelegate> delegate;
    NSArray* ListItems;
}

// Set the options of the menu, removing the previously set items
-(void)SetOptions:(NSArray*)ItemsList;

// Need to allow the calling parent to set the callback function through a delegate
// This is more "correct" than asking and saving a given object and selector, though that works too
-(void)setDelegate:(id<PopDownControllerDelegate>) givenDelegate;

@end
