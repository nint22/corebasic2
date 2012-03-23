/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#import "DebugEditorView.h"

@implementation DebugEditorView

@synthesize TextField, MainScreen, MemoryScrollView;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self) {
        
        // Default to small screen
        IsFullScreen = false;
    }
    return self;
}

- (void)Initialize
{
    // Install a push event on self so we can grow / minimize the screen
    UITapGestureRecognizer* Tapped = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(ScreenPressed:)];
    [MainScreen addGestureRecognizer:Tapped];
    
    // Initialize memory list to empty
    MemoryLines = [NSArray arrayWithObjects:@"No program yet loaded in memory...", nil];
    
    // Alloc and initialize our graphical memory output
    MemoryTable = [[UITableViewController alloc] initWithStyle:UITableViewStylePlain];
    [[MemoryTable tableView] setDelegate:self];
    [[MemoryTable tableView] setDataSource:self];
    [[MemoryTable tableView] setUserInteractionEnabled:false];
    
    // Reload / initialize empty data
    [MemoryScrollView addSubview:[MemoryTable tableView]];
    [[MemoryTable tableView] reloadData];
    
    // Reset odd positioning
    CGRect TableViewFrame = [[MemoryTable tableView] frame];
    [[MemoryTable tableView] setFrame:CGRectMake(0, 0, TableViewFrame.size.width, TableViewFrame.size.height)];
}

- (void)SetDataSource:(const char*)string
{
    // Turn into manages string to split
    NSString* DataString = [NSString stringWithCString:string encoding:NSASCIIStringEncoding];
    MemoryLines = [DataString componentsSeparatedByString:@"\n"];
    [[MemoryTable tableView] reloadData];
    
    // Change the frame so that the content can handle it
    [MemoryScrollView setContentSize:CGSizeMake(450, [MemoryLines count] * 16.0)];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    // Save text into the queue and hide keyboard
    [TextField pushMessage:[textField text] isOutput:false];
    [TextField resignFirstResponder];
    
    // Clear the text in this buffer
    [textField setText:@""];
    
    // All done
    return NO;
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    // Force update of the text field
    [TextField setNeedsDisplay];
}

- (void)ScreenPressed:(id)sender
{
    // Only on end of release
    if([sender state] != UIGestureRecognizerStateEnded)
        return;
    
    // Flip state
    IsFullScreen = !IsFullScreen;
    
    // The last full-screen frame-state
    static CGRect LastFrame;
    
    // If going full screen...
    if(IsFullScreen)
    {
        // Save last frame for use when restoring
        LastFrame = [MainScreen frame];
        
        // Find the original global position
        CGPoint OriginalPos = [MainScreen convertPoint:CGPointMake([MainScreen frame].origin.x, [MainScreen frame].origin.y) toView:nil];
        OriginalPos.y -= 20;
        
        // Remove from parent and set to parent's parent
        [MainScreen removeFromSuperview];
        [[[self superview] superview] addSubview:MainScreen];
        [MainScreen setFrame:CGRectMake(OriginalPos.x, OriginalPos.y, [MainScreen frame].size.width, [MainScreen frame].size.height)];
        
        // Target screen size when in fullscreen
        static const int ScreenWidth = 480;
        static const int ScreenHeight = 360;
        
        // Get resolution size
        CGRect ScreenSize = [[UIScreen mainScreen] bounds];
        ScreenSize.size.width *= [[UIScreen mainScreen] scale];
        ScreenSize.size.height *= [[UIScreen mainScreen] scale];
        
        // Get the world position
        [UIView animateWithDuration:0.5f animations:^{
            [MainScreen setFrame:CGRectMake(ScreenSize.size.width / 2 - ScreenWidth / 2, ScreenSize.size.height / 2 - ScreenHeight / 2, ScreenWidth, ScreenHeight)];
        }];
    }
    // Else, shrinking again
    else
    {
        // Find the localized position
        CGPoint GlobalPos = [self convertPoint:CGPointMake([MainScreen frame].origin.x, [MainScreen frame].origin.y) fromView:nil];
        GlobalPos.y += 20;
        
        // Remove from the superview and set back to this object
        [MainScreen removeFromSuperview];
        [self addSubview:MainScreen];
        [MainScreen setFrame:CGRectMake(GlobalPos.x, GlobalPos.y, [MainScreen frame].size.width, [MainScreen frame].size.height)];
        
        // Change frame
        [UIView animateWithDuration:0.5f animations:^{
            [MainScreen setFrame:LastFrame];
        }];
    }
}

/*** Memory Data Callbacks ***/

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return [MemoryLines count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:@""];
    if (cell == nil)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@""];
        [[cell textLabel] setFont:[UIFont fontWithName:@"CourierNewPS-BoldMT" size:12.0f]];
    }
    
    [[cell textLabel] setText:[MemoryLines objectAtIndex:[indexPath row]]];
    return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return 16.0f;
}

@end
