/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: UIColorTextView.h/m
 Desc: A customized UITextView that does syntax highlighting and
 helps the user do special editing features (i.e. return-tabbing).
 
***************************************************************/

#import <UIKit/UIKit.h>
#import "cbLang.h"

@interface UIColorTextView : UITextView
{
    // The parsed color highlight
    cbList TokenColors;
    
    // The colored string
    NSMutableAttributedString* ColoredString;
}

// Delegate implementation
- (void)textViewDidChange:(UITextView *)textView;

@end
