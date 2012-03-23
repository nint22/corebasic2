/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#import "LineCountViewController.h"

@implementation LineCountViewController

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
        [[self tableView] setScrollEnabled:false];
        //[[self tableView] setAllowsSelection:false];
        [[self tableView] setSeparatorColor:[UIColor clearColor]];
        
        // Set self as delegate
        [[self tableView] setDelegate:self];
        
        // No divider colors
        [[self tableView] setSeparatorColor:[UIColor clearColor]];
        [[self tableView] setBackgroundColor:[UIColor clearColor]];
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];

    // Uncomment the following line to preserve selection between presentations.
    // self.clearsSelectionOnViewWillAppear = NO;
 
    // Uncomment the following line to display an Edit button in the navigation bar for this view controller.
    // self.navigationItem.rightBarButtonItem = self.editButtonItem;
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
}

- (void)viewDidDisappear:(BOOL)animated
{
    [super viewDidDisappear:animated];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Just one section
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Max 999 lines
    return 999;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Allocate cell if needed
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:@""];
    if (cell == nil)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:@""];
        [cell setBackgroundColor:[UIColor redColor]];
        [[cell textLabel] setFont:[UIFont systemFontOfSize:10.0f]];
        [cell setBackgroundColor:[UIColor redColor]];
    }
    
    // Set cell text to just the row number
    [[cell textLabel] setText:[NSString stringWithFormat:@"%03d", [indexPath row] + 1]];
    
    // Done
    return cell;
}

/*** Delegate Functions ***/

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return 14.0f;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Turn off selection and get text debug state
    [tableView deselectRowAtIndexPath:indexPath animated:false];
    
    // Tags of 0 are false: not debugging
    UITableViewCell* ActiveCell = [tableView cellForRowAtIndexPath:indexPath];
    if([ActiveCell tag])
    {
        // Disable debugging
        [[ActiveCell textLabel] setTextColor:[UIColor blackColor]];
        [ActiveCell setTag: 0];
        
        // Remove icon
        [[ActiveCell backgroundView] removeFromSuperview];
    }
    else
    {
        // Activate debugging
        [[ActiveCell textLabel] setTextColor:[UIColor whiteColor]];
        [ActiveCell setTag: 1];
        
        // Add icon for debug line
        UIImageView* DebugIcon = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"DebugIcon.png"]];
        [DebugIcon setContentMode:UIViewContentModeCenter];
        [ActiveCell setBackgroundView:DebugIcon];
    }
}

@end
