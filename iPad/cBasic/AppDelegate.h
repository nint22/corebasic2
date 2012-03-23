/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
 File: AppDelegate.h/m
 Desc: Main application entry point / application delegate.
 Note that the true root view controller of this application is
 the UINavigationController instance "MainController", though
 the first visible controller that is pushed is a "FileBrowserController"
 
***************************************************************/

#import <UIKit/UIKit.h>
#import "FileBrowserController.h"

@interface AppDelegate : UIResponder <UIApplicationDelegate>
{
    // Main (true-root) navigation controller
    UINavigationController* MainController;
}

// Main window we work on
@property (strong, nonatomic) UIWindow *window;

// Initialize application properly
-(void)ApplicationInitialize;

@end
