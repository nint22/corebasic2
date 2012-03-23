/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: ConsoleTextView.h/m
 Desc: A customized UITextView that deals with special-rendering
 of the application console. Output text is bolded, while user
 input is just plain monospace font.
 
***************************************************************/

#import <UIKit/UIKit.h>
#import <CoreText/CoreText.h>

@interface ConsoleTextView : UITextView
{
    // The colored string
    NSMutableAttributedString* ColoredString;
    
    // List of console strings which is a tuple of a boolean
    // being either true (that it is console out) or false (it is what the user wrote)
    NSMutableArray* ConsoleText;
    
    // A special queue used for reading user input
    NSMutableArray* InputQueue;
}

// Add an output console message
-(void)pushMessage:(NSString*) Message isOutput:(bool)IsOutput;

// Get the latest string the user put in
-(NSString*) getMessage;

@end
