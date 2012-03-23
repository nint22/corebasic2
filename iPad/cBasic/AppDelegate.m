/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#import "AppDelegate.h"
#import "EditorViewController.h"
#import "FileBrowserController.h"

@implementation AppDelegate

@synthesize window = _window;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // First time we are launching the application?
    [[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithBool:YES],@"firstLaunch",nil]];
    if([[NSUserDefaults standardUserDefaults] boolForKey:@"firstLaunch"])
        [self ApplicationInitialize];
    
    // Allocate the window
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    
    // Allocate the file viewer and push it onto the view-stack
    FileBrowserController* FileBrowser = [[FileBrowserController alloc] initWithNibName:@"FileBrowserController" bundle:nil];
    [FileBrowser setDirectory:@"cBasic"];
    
    // Initialize with a navigation controller as the root
    MainController = [[UINavigationController alloc] initWithRootViewController:FileBrowser];
    [MainController setNavigationBarHidden:true];
    
    // Add to main parent / window
    [self.window addSubview:[MainController view]];
    //[[MainController view] setFrame:[[self.window screen] applicationFrame]];
    
    // Override point for customization after application launch.
    self.window.backgroundColor = [UIColor blackColor];
    [self.window makeKeyAndVisible];
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    /*
     Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
     If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
     */
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    /*
     Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
     */
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    /*
     Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
     */
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    /*
     Called when the application is about to terminate.
     Save data if appropriate.
     See also applicationDidEnterBackground:.
     */
}

-(void)ApplicationInitialize
{
    // Allocate the root cBasic folder
    NSString* DocumentsPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
    [[NSFileManager defaultManager] createDirectoryAtPath:[NSString stringWithFormat:@"%@/cBasic", DocumentsPath] withIntermediateDirectories:false attributes:nil error:nil];
    
    // Allocate the root trash folder
    [[NSFileManager defaultManager] createDirectoryAtPath:[NSString stringWithFormat:@"%@/cBasic/Trash", DocumentsPath] withIntermediateDirectories:false attributes:nil error:nil];
    
    // Copy examples and challenges
    for(int i = 0; i < 2; i++)
    {
        // Folder name
        NSString* FolderName = @"Examples";
        if(i == 1)
            FolderName = @"Challenges";
        
        // Create the examples directory
        [[NSFileManager defaultManager] createDirectoryAtPath:[NSString stringWithFormat:@"%@/cBasic/%@", DocumentsPath, FolderName] withIntermediateDirectories:false attributes:nil error:nil];
        
        // Copy all examples from the bundle into this directory
        NSArray* BundleContents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath: [[NSBundle mainBundle] bundlePath] error:nil];
        
        // For each item in this bundle, copy to docs path
        for(NSString* FileName in BundleContents)
        {
            // Must start with the folder name and end in *.cb
            if(![[FileName lowercaseString] hasPrefix:[FolderName lowercaseString]] || ![[FileName lowercaseString] hasSuffix:@".cb"])
               continue;
            
            // Copy over file
            NSString* FromPath = [NSString stringWithFormat:@"%@/%@", [[NSBundle mainBundle] bundlePath], FileName];
            NSString* ToPath = [NSString stringWithFormat:@"%@/cBasic/%@/%@", DocumentsPath, FolderName, FileName];
            [[NSFileManager defaultManager] copyItemAtPath:FromPath toPath:ToPath error:nil];
        }
    }
    
    // No longer the first time we launched this application
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"firstLaunch"];
}

@end
