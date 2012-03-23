/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: ContextMenu.h/m
 Desc: Renders a circular context menu object, which allows the
 user to select keywords of the cBasic programming language
 based on gestures. The context menu is opened by doing a
 circle gesture, then the root group type is selected by a drag
 in a certain direction and finally a tap in the sub-category.
 
 + Conditional
   - if
   - elif
   - else
   - and
   - or
   - !
   - Comparisons (<=, etc..)
 + Control
   - label
   - goto
   - while
   - for
   - Functions (exec, func)
 + Maths
   - =, +, -, *, /, %
 + I/O
   - input
   - disp
   - pause
   - getKey
 
***************************************************************/

#import <UIKit/UIKit.h>

@interface ContextMenu : UIView

@end
