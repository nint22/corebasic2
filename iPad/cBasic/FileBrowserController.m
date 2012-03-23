/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#import "FileBrowserController.h"

@implementation FileBrowserController

@synthesize MajorLabel, MinorLabel;
@synthesize BackButton, AddButton;
@synthesize ScrollView;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
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
    
    // Explicitly re-load the directory we want
    [self loadDirectory];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
    // Release any retained subviews of the main view.
    // e.g. self.myOutlet = nil;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    // Return YES for supported orientations
	return YES;
}

- (IBAction)BackPressed:(id)Sender
{
    // Pop off self
    [[self navigationController] popViewControllerAnimated:true];
}

- (IBAction)AddFilePressed:(id)Sender
{
    // Have a simple pop-over appear
    PopoverSelector = [[PopDownController alloc] initWithStyle:UITableViewStylePlain];
    [PopoverSelector SetOptions:[NSArray arrayWithObjects:@"Create file...", @"Create folder...", nil]];
    [PopoverSelector setDelegate:self];
    
    // Actual popover
    Popover = [[UIPopoverController alloc] initWithContentViewController:PopoverSelector];
    [Popover setPopoverContentSize:CGSizeMake(280, 64)];
    [Popover presentPopoverFromRect:[Sender frame] inView:[self view] permittedArrowDirections:UIPopoverArrowDirectionAny animated:true];
}

- (IBAction)HelpPressed:(id)Sender
{
    /*** Create Web Browser ***/
    
    // Get the url of "help.htm"
    NSURL* HelpDoc = [[NSBundle mainBundle] URLForResource:@"help" withExtension:@"htm"];
    
    // Create a parent view controller
    WebViewController = [[UIViewController alloc] init];
    
    // If the user presses help, create a web browser and load the help doc
    UIWebView* HelpView = [[UIWebView alloc] init];
    [HelpView loadRequest:[NSURLRequest requestWithURL:HelpDoc]];
    
    // Have the web controller host the web view
    [WebViewController setView:HelpView];
    
    /*** Create Control Buttons ***/
    
    // Add back button
    CGSize ButtonRect = [[UIImage imageNamed:@"StepOut_Active.png"] size];
    WebBackButton = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, ButtonRect.width, ButtonRect.height)];
    
    // Change states for the images
    [WebBackButton setImage:[UIImage imageNamed:@"StepOut_Active"] forState:UIControlStateNormal];
    [WebBackButton setImage:[UIImage imageNamed:@"StepOut_Pushed"] forState:UIControlStateSelected];
    
    // Set callback
    [WebBackButton addTarget:self action:@selector(BackPressed:) forControlEvents:UIControlEventTouchUpInside];
    
    // Add button to view
    [HelpView addSubview:WebBackButton];
    
    /*** Done ***/
    
    // Push onto view
    [[self navigationController] pushViewController:WebViewController animated:true];
}

- (void) setDirectory: (NSString*)DirectoryName
{
    // Save and get the directory content (rendering it as well)
    DirName = DirectoryName;
}

- (void) loadDirectory
{
    // Special rule: if we are in the root directory (i.e. it is just "cBasic" then disable the back button
    [BackButton setEnabled:!([DirName compare:@"cBasic"] == NSOrderedSame)];
    
    // Remove all previous icons
    NSArray* Subviews = [ScrollView subviews];
    for(id Subview in Subviews)
    {
        if([Subview isKindOfClass:[UIView class]])
            [(UIView*)Subview removeFromSuperview];
    }
    
    // Load all the paths of the documents directory
    NSString* DocumentsPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
    
    // Load the image for both the file and folder icon
    UIImage* FolderIcon = [UIImage imageNamed:@"FolderIcon.png"];
    UIImage* FileIcon = [UIImage imageNamed:@"FileIcon.png"];
    UIImage* TrashIcon = [UIImage imageNamed:@"TrashIcon.png"];
    
    // Load all file items in the directory
    NSError* Error = nil;
    NSString* FullFileDir = [NSString stringWithFormat:@"%@/%@", DocumentsPath, DirName];
    DirectoryContents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:FullFileDir error:&Error];
    NSAssert(Error == nil, @"Cannot access empty directory");
    
    // Special rule: if we are in the trash, challenges, or examples, then disable add files
    if([FullFileDir hasPrefix:[NSString stringWithFormat:@"%@/cBasic/Challenges", DocumentsPath]] ||
       [FullFileDir hasPrefix:[NSString stringWithFormat:@"%@/cBasic/Examples", DocumentsPath]] ||
       [FullFileDir hasPrefix:[NSString stringWithFormat:@"%@/cBasic/Trash", DocumentsPath]])
        [AddButton setEnabled:false];
    
    int DirectoryCount = 0, FileCount = 0, FolderCount = 0;
    for(NSString* FileName in DirectoryContents)
    {
        // Get the file information
        BOOL IsFolder;
        NSString* FullFileName = [NSString stringWithFormat:@"%@/%@", FullFileDir, FileName];
        [[NSFileManager defaultManager] fileExistsAtPath:FullFileName isDirectory:&IsFolder];
        
        // Count files and directories
        if(IsFolder)
            FolderCount++;
        else
            FileCount++;
        
        // Load an icon
        UIImageView* IconView = nil;
        if([FullFileName compare:[NSString stringWithFormat:@"%@/%@", DocumentsPath, @"cBasic/Trash"]] == 0)
            IconView = [[UIImageView alloc] initWithImage:TrashIcon];
        else if(IsFolder)
            IconView = [[UIImageView alloc] initWithImage:FolderIcon];
        else
            IconView = [[UIImageView alloc] initWithImage:FileIcon];
        
        // Generate a title for the file name
        UILabel* LabelView = [[UILabel alloc] init];
        [LabelView setText:FileName];
        [LabelView setBackgroundColor:[UIColor colorWithRed:0 green:0 blue:0 alpha:0.05f]];
        [LabelView setTextAlignment:UITextAlignmentCenter];
        [LabelView setLineBreakMode:UILineBreakModeTailTruncation];
        
        // Position both as needed
        static const int MaxElements = 6;
        int x = DirectoryCount % MaxElements * 128 + 40;
        int y = (DirectoryCount / MaxElements) * 100 + 20;
        [IconView setFrame:CGRectMake(x, y, 48, 48)];
        [LabelView setFrame:CGRectMake(x + 48 / 2 - 50, y + 50, 100, 20)];
        
        // Add to view
        [ScrollView addSubview:IconView];
        [ScrollView addSubview:LabelView];
        
        // Create a call back for when the user holds too long down
        UILongPressGestureRecognizer* IconLongPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(removePushed:)];
        UITapGestureRecognizer* IconTapPress = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(filePushed:)];
        UILongPressGestureRecognizer* LabelLongPress = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(removePushed:)];
        UITapGestureRecognizer* LabelTapPress = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(filePushed:)];
        
        // Add callbacks for both the icon and the text
        [IconView setUserInteractionEnabled:true];
        [IconView addGestureRecognizer:IconLongPress];
        [IconView addGestureRecognizer:IconTapPress];
        
        [LabelView setUserInteractionEnabled:true];
        [LabelView addGestureRecognizer:LabelLongPress];
        [LabelView addGestureRecognizer:LabelTapPress];
        
        // Callback details to help recieving functions
        [IconView setTag:DirectoryCount];
        [LabelView setTag:DirectoryCount];
        
        // Grow the element count
        DirectoryCount++;
    }
    
    // Update the top labels with this information
    [MajorLabel setText:[NSString stringWithFormat:@"\"%@\" Directory", DirName]];
    if(FileCount > 0 || FolderCount > 0)
        [MinorLabel setText:[NSString stringWithFormat:@"%d Files, %d Folders", FileCount, FolderCount]];
    else
        [MinorLabel setText:@"Empty Directory"];
}

-(void)itemSelected:(int)index
{
    // Close the popup
    [Popover dismissPopoverAnimated:true];
    
    // 0 is file, 1 is folder
    NSString* Message = nil;
    if(index == 0)
    {
        Message = @"Enter file name";
        PopoverState = PopoverState_NewFile;
    }
    else
    {
        Message = @"Enter folder name";
        PopoverState = PopoverState_NewFolder;
    }
    
    // Taken from http://stackoverflow.com/questions/4467379/how-to-popup-a-text-field
    UIAlertView* prompt = [[UIAlertView alloc] initWithTitle:Message message:@"\n\n" delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Continue", nil];
    
    // Put a text field
    PopoverTextField = [[UITextField alloc] initWithFrame:CGRectMake(12, 50, 260, 25)];
    [PopoverTextField setBackgroundColor:[UIColor whiteColor]];
    [prompt addSubview:PopoverTextField];
    
    // Catch the user's edit-commit
    [prompt setDelegate:self];
    
    // Show the dialog box and focus on it
    [prompt show];
    [PopoverTextField becomeFirstResponder];
}

- (void) removePushed: (UILongPressGestureRecognizer*)sender
{
    // We only care on initial push
    if([sender state] != UIGestureRecognizerStateBegan)
        return;
    
    // What object did we select
    int ItemID = [[sender view] tag];
    PopoverState = PopoverState_DeleteItem;
    
    // What is the file name?
    NSString* FileName = [DirectoryContents objectAtIndex:ItemID];
    
    // Do a pop-up to confirm the delection
    UIAlertView* prompt = [[UIAlertView alloc] initWithTitle:@"Delete file?" message:[NSString stringWithFormat:@"Would you really like to delete the file or folder \"%@\"?", FileName] delegate:self cancelButtonTitle:@"Cancel" otherButtonTitles:@"Delete", nil];
    [prompt setTag:ItemID];
    [prompt show];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    // Ignore if anything except accept
    if(buttonIndex != 1)
        return;
    
    // Error handle
    NSError* Error = nil;
    
    /*** Create Item ***/
    
    if(PopoverState == PopoverState_NewFile || PopoverState == PopoverState_NewFolder)
    {
        // Create folder path
        NSString* DocumentsPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
        NSString* FileName = [PopoverTextField text];
        
        // Exception, if we are creating a file, just add extension
        if(PopoverState == PopoverState_NewFile && ![[FileName pathExtension] isEqualToString:@"cb"])
            FileName = [NSString stringWithFormat:@"%@.cb", FileName];
        
        NSString* DirectoryName = [NSString stringWithFormat:@"%@/%@/%@", DocumentsPath, DirName, FileName];
        
        // Ignore if we already have a collision
        BOOL IsDirectory = false;
        BOOL IsFile = [[NSFileManager defaultManager] fileExistsAtPath:DirectoryName isDirectory:&IsDirectory];
        
        // If either are true, fail out
        if(IsFile || IsDirectory)
        {
            UIAlertView* Popup = [[UIAlertView alloc] initWithTitle:@"Error" message:[NSString stringWithFormat:@"A %@ is already named \"%@\"", IsFile ? @"file" : @"directory", FileName] delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
            [Popup show];
        }
        else
        {
            // Are we creating a file or folder?
            if(PopoverState == PopoverState_NewFolder)
                [[NSFileManager defaultManager] createDirectoryAtPath:DirectoryName withIntermediateDirectories:false attributes:nil error:&Error];
            // Else, create file
            else
                [[NSFileManager defaultManager] createFileAtPath:DirectoryName contents:[@"// Empty application\ndisp(\"Hello, World!\\n\")" dataUsingEncoding:NSUTF8StringEncoding] attributes:nil];
            
            // If we get any errors, it isn't fatal, just print what the issue is
            if(Error != nil)
            {
                UIAlertView* Popup = [[UIAlertView alloc] initWithTitle:@"Error" message:@"Invalid file or folder name." delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
                [Popup show];
            }
        }
    }
    
    /*** Deleting ***/
    
    else
    {
        // Create the file name
        int ItemID = [alertView tag];
        NSString* FileName = [DirectoryContents objectAtIndex:ItemID];
        NSString* FullFileName = [NSString stringWithFormat:@"%@/%@/%@", [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject], DirName, FileName];
        NSString* TargetFile = [NSString stringWithFormat:@"%@/%@", DirName, FileName];
        
        // Is the user attempting to delete an example, challenge, or garbage?
        if([TargetFile hasPrefix:@"cBasic/Examples"] || [TargetFile hasPrefix:@"cBasic/Challenges"] || [TargetFile compare:@"cBasic/Trash"] == 0)
        {
            //Fail out
            UIAlertView* Popup = [[UIAlertView alloc] initWithTitle:@"Error" message:@"Cannot delete examples, challenges, or the trash folder." delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
            [Popup show];
        }
        // If attempting to delete from trash, go ahead
        else if([TargetFile hasPrefix:@"cBasic/Trash"])
        {
            // Attempt to delete the file...
            NSError* Error = nil;
            [[NSFileManager defaultManager] removeItemAtPath:FullFileName error:&Error];
            
            // Error check
            NSAssert(Error == nil, @"Unable to delete \"%@\".", FileName);
        }
        // Else, move to trash
        else
        {
            // Create date-time to tag trashed files
            NSDateFormatter* DateTimeFormat = [[NSDateFormatter alloc] init];
            [DateTimeFormat setDateFormat:@"MMM-dd-yyyy HH-mm"];
            
            // Attempt to move file to trash
            NSError* Error = nil;
            NSString* TargetPath = [NSString stringWithFormat:@"%@/cBasic/Trash/%@ - %@", [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject], FileName, [DateTimeFormat stringFromDate:[[NSDate alloc] init]]];
            [[NSFileManager defaultManager] moveItemAtPath:FullFileName toPath:TargetPath error:&Error];
            
            // Error check
            NSAssert(Error == nil, @"Unable to delete \"%@\": %@", FileName, [Error localizedDescription]);
        }
    }
    
    // Reload the directory
    [self loadDirectory];
}

- (void)filePushed:(UITapGestureRecognizer*)sender
{
    // Only on end of release
    if([sender state] != UIGestureRecognizerStateEnded)
        return;
    
    // Create item path
    NSString* DocumentsPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
    NSString* FileName = [DirectoryContents objectAtIndex:[[sender view] tag]];
    NSString* DirectoryName = [NSString stringWithFormat:@"%@/%@/%@", DocumentsPath, DirName, FileName];
    
    // Ignore if we already have a collision
    BOOL IsDirectory = false;
    BOOL IsFile = [[NSFileManager defaultManager] fileExistsAtPath:DirectoryName isDirectory:&IsDirectory];
    
    // Did we push a file or a folder?
    if(IsFile && !IsDirectory)
    {
        // Load file as an editing view
        EditorViewController* NewEditor = [[EditorViewController alloc] initWithNibName:@"EditorViewController" bundle:nil];
        [NewEditor LoadFile:[NSString stringWithFormat:@"%@/%@", DirName, FileName]];
        
        // Push onto view controller stack
        [[self getNavigationController] pushViewController:NewEditor animated:true];
    }
    // Else is directory
    else if(IsFile && IsDirectory)
    {
        // Add a new file browser view of this directory
        FileBrowserController* NewFileBrowser = [[FileBrowserController alloc] initWithNibName:@"FileBrowserController" bundle:nil];
        [NewFileBrowser setDirectory:[NSString stringWithFormat:@"%@/%@", DirName, FileName]];
        
        // Push onto view controller stack
        [[self getNavigationController] pushViewController:NewFileBrowser animated:true];
    }
    // Else never should reach here
    else
        NSAssert(false, @"Invalid item selected");
}

- (UINavigationController*) getNavigationController
{
    // Keep seeking up the chain of view-controller parents until the navigation controller is found
    id Parent = [self parentViewController];
    while(![Parent isKindOfClass:[UINavigationController class]] && Parent != nil)
        Parent = [Parent parentViewController];
    return Parent;
}

@end
