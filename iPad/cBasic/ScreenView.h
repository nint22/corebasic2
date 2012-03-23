/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: EditorViewController.h/m
 Desc: The 4-bit 96x64 resolution screen found in the bottom
 left of the application. Note that the entire screen color is
 gray / brown to mimick only LCD screens. Also note that the
 screen does not render unless it has been updated.
 
***************************************************************/

#import <UIKit/UIKit.h>

// The type of color
typedef enum __ScreenView_Color
{
    ScreenView_Color_0 = 0, // Black (no color)
    ScreenView_Color_1,
    ScreenView_Color_2,
    ScreenView_Color_3,     // White (full color)
} ScreenView_Color;

// Define screen resolution
static const int ScreenView_ScreenWidth = 96;
static const int ScreenView_ScreenHeight = 64;

@interface ScreenView : UIView
{
    // Pixel data
    char* ScreenBuffer;
}

// Set the pixel color at the given pixel coordinate
-(void) setPixel: (ScreenView_Color)Color atX: (int)x atY: (int) y;

// Clear the screen
-(void) clearScreen;

@end
