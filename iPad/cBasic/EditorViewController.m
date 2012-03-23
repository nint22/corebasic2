/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#import "EditorViewController.h"

@implementation EditorViewController

@synthesize MenuButton, RunButton, StopButton;
@synthesize StepOverButton, StepIntoButton, StepOutButton;
@synthesize EditorView, DebugView;
@synthesize TopDiv, BottomDiv, TopLabel, BottomLabel;
@synthesize TopBuildLabel, BottomBuildLabel;
@synthesize BuildIcon;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self)
    {
        // Initialize a date-time format
        DateTimeFormat = [[NSDateFormatter alloc] init];
        [DateTimeFormat setDateFormat:@"MMM dd, yyyy HH:mm"];
        
        // Initialize the GUI timer with nothing just in case it gets called too soon
        GUITimer = nil;
    }
    return self;
}

- (void)didReceiveMemoryWarning
{
    // Releases the view if it doesn't have a superview.
    [super didReceiveMemoryWarning];
    
    // Release any cached data, images, etc that aren't in use.
}

-(void)dealloc
{
    // Sanity check.
    NSLog(@"Sanity check");
}

#pragma mark - View lifecycle

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    // Default with debug elements disabled
    [StepOverButton setEnabled:false];
    [StepIntoButton setEnabled:false];
    [StepOutButton setEnabled:false];
    
    // Default with stop disabled
    [StopButton setEnabled:false];
    
    // Initialize the editor view 
    ViewEditor = [[[NSBundle mainBundle] loadNibNamed:@"TextEditorView" owner:self options:nil] objectAtIndex:0];
    [ViewEditor initialize: self];
    [EditorView addSubview:ViewEditor];
    
    // Re-size as needed
    CGRect Shape = [EditorView frame];
    Shape.origin = CGPointMake(0.0f, 0.0f);
    [ViewEditor setFrame: Shape];
    
    // Initialize the debug view
    ViewDebug = [[[NSBundle mainBundle] loadNibNamed:@"DebugEditorView" owner:self options:nil] objectAtIndex:0];
    [ViewDebug Initialize];
    [DebugView addSubview:ViewDebug];
    
    // Re-size as needed
    Shape = [DebugView frame];
    Shape.origin = CGPointMake(0.0f, 0.0f);
    [ViewDebug setFrame: Shape];
    
    // Load the text editor and force a graphical update
    [ViewEditor setText:[NSString stringWithContentsOfFile:FileName encoding:NSUTF8StringEncoding error:nil]];
    [ViewEditor textViewDidChange:[ViewEditor TextField]];
    [TopLabel setText:ShortFileName];
    
    // Force-save self so we update the GUI
    [self SaveFile];
    
    // Show on the console the code has been loaded
    unsigned int Major, Minor;
    cbGetVersion(&Major, &Minor);
    [[ViewDebug TextField] pushMessage:[NSString stringWithFormat:@"cBasic %d.%d - Code loaded...\n", Major, Minor] isOutput:true];
    
    // Allow user to drag bottom view
    [TopDiv setUserInteractionEnabled:true];
    UILongPressGestureRecognizer* Gesture = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(debugDragging:)];
    [TopDiv addGestureRecognizer:Gesture];
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

-(void) keyboardWillShow:(CGRect) KeyboardSize
{
    [self animateOffset:(int)KeyboardSize.size.height];
}

-(void) keyboardWillHide:(CGRect) KeyboardSize
{
    [self animateOffset:-(int)KeyboardSize.size.height];
}

-(void) animateOffset:(int) KeyboardHeight
{
    // Shrink the text editor
    CGRect TextFieldFrame = [EditorView frame];
    TextFieldFrame.size.height -= KeyboardHeight;
    
    // Move the debug up
    CGRect DebugFrame = [DebugView frame];
    DebugFrame.origin.y -= KeyboardHeight;
    
    // Move both image bars up
    CGRect TopImage = [TopDiv frame];
    TopImage.origin.y -= KeyboardHeight;
    CGRect BottomImage = [BottomDiv frame];
    BottomImage.origin.y -= KeyboardHeight;
    CGRect BottomLabelFrame = [BottomLabel frame];
    BottomLabelFrame.origin.y -= KeyboardHeight;
    
    // Animate
    [UIView animateWithDuration:0.3f animations:^{
        [EditorView setFrame:TextFieldFrame];
        [DebugView setFrame:DebugFrame];
        [TopDiv setFrame:TopImage];
        [BottomDiv setFrame:BottomImage];
        [BottomLabel setFrame:BottomLabelFrame];
    }];
}

-(IBAction)RunPressed:(id)sender
{
    // Lock the run-button, and enable the stop button
    [RunButton setEnabled:false];
    [StopButton setEnabled:true];
    [MenuButton setEnabled:false];
    [[ViewEditor TextField] setEditable:false];
    
    // Commit the code and save
    [self SaveFile];
    
    // Update the label strings
    [TopBuildLabel setText:@"Build cBasic: Strating build..."];
    
    // Compile and simulate the code
    [self performSelectorInBackground:@selector(compileCode:) withObject:[[ViewEditor TextField] text]];
}

-(IBAction)MenuPressed:(id)sender
{
    // Have a simple pop-over appear
    PopoverSelector = [[PopDownController alloc] initWithStyle:UITableViewStylePlain];
    [PopoverSelector SetOptions:[NSArray arrayWithObjects:@"\u21A9 Undo Change", @"\u21AA Redo Change", @"\u21CA Save", @"\u21EA Quit", nil]];
    [PopoverSelector setDelegate:self];
    
    // Actual popover
    Popover = [[UIPopoverController alloc] initWithContentViewController:PopoverSelector];
    [Popover setPopoverContentSize:CGSizeMake(280, 128)];
    [Popover presentPopoverFromRect:[sender frame] inView:[self view] permittedArrowDirections:UIPopoverArrowDirectionUp animated:true];
}
    
-(void)itemSelected:(int)index
{
    // 0: Undo, 1: redo
    if(index == 0)
    {
        if(![ViewEditor Undo])
        {
            UIAlertView* Popup = [[UIAlertView alloc] initWithTitle:@"Undo Error" message:@"Nothing to undo" delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
            [Popup show];
        }
    }
    else if(index == 1)
    {
        if(![ViewEditor Redo])
        {
            UIAlertView* Popup = [[UIAlertView alloc] initWithTitle:@"Redo Error" message:@"Nothing to redo" delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
            [Popup show];
        }
    }
    
    // Force-save of the document
    else if(index == 2)
        [self SaveFile];
    
    // Back to file manager
    else if(index == 3)
    {
        [self SaveFile];
        [[self navigationController] popViewControllerAnimated:true];
    }
    
    // Choice was made by the user pressing on PopoverSelector
    [Popover dismissPopoverAnimated:true];
}

-(IBAction)StopPressed:(id)sender
{
    // Halt simulation
    [GUITimer invalidate];
    
    // Turn on buttons again
    [RunButton setEnabled:true];
    [StopButton setEnabled:false];
    [MenuButton setEnabled:true];
    [[ViewEditor TextField] setEditable:true];
    
    // Release process
    cbRelease(&Processor);
    
    // Close file streams
    fclose(StreamOut);
    fclose(StreamIn);
    
    // Post that we halted the process
    NSDate* Today = [[NSDate alloc] init];
    [TopBuildLabel setText:[NSString stringWithFormat: @"Stopped cBasic | User-halted, %.2f seconds", (float)(Today.timeIntervalSince1970 - SimulationStart.timeIntervalSince1970)]];
    [BuildIcon setImage:[UIImage imageNamed:@"Icon_Warning.png"]];
}

-(void) LoadFile:(NSString*) SourceFileName
{
    NSString* DocumentsPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
    FileName = [NSString stringWithFormat:@"%@/%@", DocumentsPath, SourceFileName];
    ShortFileName = SourceFileName;
}

-(void) SaveFile
{
    // Write to fine
    NSError* Error = nil;
    [[[ViewEditor TextField] text] writeToFile:FileName atomically:false encoding:NSASCIIStringEncoding error:&Error];
    
    // If we ever fail, tell the user
    if(Error != nil)
    {
        UIAlertView* Popup = [[UIAlertView alloc] initWithTitle:@"Build Error" message:[NSString stringWithFormat:@"Unable to write to file: %@", [Error description]] delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
        [Popup show];
    }
    // Else, update the top text
    else
    {
        // Update the subline for the top-build view
        int LinesOfCode = [[[[ViewEditor TextField] text] componentsSeparatedByString:@"\n"] count];
        [BottomBuildLabel setText:[NSString stringWithFormat:@"%d Lines of Code | Last Saved: %@", LinesOfCode, [DateTimeFormat stringFromDate:[[NSDate alloc] init]]]];
    }
}

-(void) compileCode: (NSString*) Code
{
    // File streams for input / output (must be allocated first since we are sharing)
    StreamOut = tmpfile();
    StreamIn = tmpfile();
    StreamReadPos = 0;
    
    // Start compiling code... (2 meg ram)
    const char* SourceCode = [Code UTF8String];
    SimulatorError = cbInit_LoadSource(&Processor, 2048, SourceCode, StreamOut, StreamIn, ScreenView_ScreenWidth, ScreenView_ScreenHeight);
    InterruptState = cbInterrupt_None;
    
    // Was there any sort of error?
    if(SimulatorError != cbError_NoError)
    {
        // Turn on buttons again
        [RunButton setEnabled:true];
        [StopButton setEnabled:false];
        [MenuButton setEnabled:true];
        [[ViewEditor TextField] setEditable:true];
        
        // Build failed
        [TopBuildLabel setText:[NSString stringWithFormat:@"Build cBasic: Failed | %s..", cbErrorNames[SimulatorError]]];
        [BuildIcon setImage:[UIImage imageNamed:@"Icon_Warning.png"]];
        
        // Do a pop-up error
        UIAlertView* Popup = [[UIAlertView alloc] initWithTitle:@"Build Error" message:[NSString stringWithCString:cbErrorNames[SimulatorError] encoding:NSASCIIStringEncoding] delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil];
        [Popup show];
        
        // Release process
        cbRelease(&Processor);
        
        // Close file streams
        fclose(StreamOut);
        fclose(StreamIn);
    }
    else
    {
        // Build succeeded
        [TopBuildLabel setText:@"Running cBasic | Run-Time: 0 seconds"];
        [BuildIcon setImage:[UIImage imageNamed:@"Icon_Success.png"]];
        
        /** Load memory map **/
        
        // Pull out the memory segment
        FILE* DebugFile = tmpfile();
        cbDebug_PrintMemory(&Processor, DebugFile);
        fseek(DebugFile, 0L, SEEK_END);
        
        // Read into a single string
        int Length = ftell(DebugFile);
        fseek(DebugFile, 0L, SEEK_SET);
        char* TextBuffer = malloc(Length + 1);
        fread(TextBuffer, Length, 1, DebugFile);
        TextBuffer[Length] = '\0';
        
        // Pass the data
        [ViewDebug SetDataSource:TextBuffer];
        
        // Done with the file
        free(TextBuffer);
        fclose(DebugFile);
        
        /** Load instructions list **/
        
        // Pull out the memory segment
        DebugFile = tmpfile();
        cbDebug_PrintInstructions(&Processor, DebugFile);
        fseek(DebugFile, 0L, SEEK_END);
        
        // Read into a single string
        Length = ftell(DebugFile);
        fseek(DebugFile, 0L, SEEK_SET);
        TextBuffer = malloc(Length + 1);
        fread(TextBuffer, Length, 1, DebugFile);
        TextBuffer[Length] = '\0';
        
        // Pass the data
        [ViewEditor SetInstructionsSource:TextBuffer];
        
        // Done with the file
        free(TextBuffer);
        fclose(DebugFile);
        
        /** Launch Simulation **/
        
        // Save the simulation start time
        SimulationStart = [[NSDate alloc] init];
        
        // Launch main-thread simulation
        [GUITimer fire];
    }
    
    // Launch simulation
    [self performSelectorOnMainThread:@selector(launchSimulation:) withObject:self waitUntilDone:false];
}

-(void) launchSimulation: (id)sender
{
    // This timer must launch from the main thread, but does not yet start 
    // Current simulation speed is 1kHz
    GUITimer = [NSTimer scheduledTimerWithTimeInterval:0.001f target:self selector:@selector(simulateCode:) userInfo:nil repeats:true];
}

-(void) simulateCode: (NSTimer*)sender
{
    /*** Input wanted from Simulation ***/
    
    // If interrupted for input
    if(SimulatorError == cbError_NoError && InterruptState != cbInterrupt_None)
    {
        // If waiting for full input line..
        if(InterruptState == cbInterrupt_Input)
        {
            // Get current input
            NSString* UserInput = [[ViewDebug TextField] getMessage];
            char input[256] = "";
            
            // If valid, copy, else give up until input
            if(UserInput != NULL)
                strncpy(input, [UserInput UTF8String], 255);
            else
                return;
            
            // Post the result
            cbStep_ReleaseInterrupt(&Processor, input);
            InterruptState = cbInterrupt_None;
        }
        // Else, not yet supported
        else
        {
            [[ViewDebug TextField] pushMessage:@"WARNING: Unable to handle this type of interrupt\n" isOutput:true];
            cbStep_ReleaseInterrupt(&Processor, "");
            InterruptState = cbInterrupt_None;
        }
    }
    
    // Keep simulating
    else if(SimulatorError == cbError_NoError)
    {
        /*** Simulate ***/
        
        // Step (must lock for file I/O)
        SimulatorError = cbStep(&Processor, &InterruptState);
        
        /*** Screen output from Simulation ***/
        
        // For any and all new drawing events
        unsigned int x;
        unsigned int y;
        unsigned int c;
        while(cbStep_GetOutput(&Processor, &x, &y, &c))
        {
            // If max, it's a clear event
            if(x == UINT_MAX && y == UINT_MAX && c == UINT_MAX)
                [[ViewDebug MainScreen] clearScreen];
            else
                [[ViewDebug MainScreen] setPixel:c atX:x atY:y];
        }
        
        /*** Output from Simulation ***/
        
        // Save the current write-pointer
        int writePos = ftell(StreamOut);
        fseek(StreamOut, StreamReadPos, SEEK_SET);
        
        // Anything new in the file output sream?
        char buffer[512];
        size_t bufferLength = fread(buffer, 1, 512, StreamOut);
        if(bufferLength > 0)
        {
            // Cap buffer, push to stdout, and then move the read
            // pos so we don't re-read what we have already read
            buffer[bufferLength] = '\0';
            StreamReadPos += bufferLength;
            
            // Push to the text console output
            [[ViewDebug TextField] pushMessage:[NSString stringWithCString:buffer encoding:NSASCIIStringEncoding] isOutput:true];
        }
        
        // Set the write-pointer back
        fseek(StreamOut, (long)writePos, SEEK_SET);
        
        // Still running, update clock
        NSDate* Today = [[NSDate alloc] init];
        [TopBuildLabel setText:[NSString stringWithFormat:@"Running cBasic | Running %d seconds", (int)(Today.timeIntervalSince1970 - SimulationStart.timeIntervalSince1970)]];
    }
    // Else, this step failed
    else
    {
        // Process crashed
        if(SimulatorError != cbError_NoError && SimulatorError != cbError_Halted)
        {
            [TopBuildLabel setText:[NSString stringWithFormat: @"Stopped cBasic | Crashed: %c", cbErrorNames[SimulatorError]]];
            [BuildIcon setImage:[UIImage imageNamed:@"Icon_Error.png"]];
        }
        
        // Else, process completed correctly
        else
        {
            NSDate* Today = [[NSDate alloc] init];
            [TopBuildLabel setText:[NSString stringWithFormat: @"Stopped cBasic | Terminated Normally, %.2f seconds", (float)(Today.timeIntervalSince1970 - SimulationStart.timeIntervalSince1970)]];
        }
        
        // Turn on buttons again
        [RunButton setEnabled:true];
        [StopButton setEnabled:false];
        [MenuButton setEnabled:true];
        [[ViewEditor TextField] setEditable:true];
        
        // Release process
        cbRelease(&Processor);
        
        // Close file streams
        fclose(StreamOut);
        fclose(StreamIn);
        
        // Stop this timer
        [sender invalidate];
    }
}

-(void) debugDragging:(UILongPressGestureRecognizer*)sender
{
    // Drag being held down
    if([sender state] == UIGestureRecognizerStateChanged)
    {
        // Get the view height and current offset
        float OffsetY = [sender locationInView:[self view]].y;
        float ViewHeight = [[self view] frame].size.height;
        
        // What is the offset between the top bar and the debug view
        float DebugDelta = fabs([DebugView frame].origin.y - [TopDiv frame].origin.y);
        
        // Ignore if too high or too low
        if(OffsetY <= 500 || OffsetY >= 780)
            return;
        
        /** Drag views **/
        
        // Shrink the text editor
        CGRect TextFieldFrame = [EditorView frame];
        TextFieldFrame.size.height = OffsetY;
        
        // Move the debug up and resize as needed
        CGRect DebugFrame = [DebugView frame];
        DebugFrame.origin.y = OffsetY;
        DebugFrame.size.height = ViewHeight - OffsetY - [BottomDiv frame].size.height;
        
        // Move top image bar up
        CGRect TopImage = [TopDiv frame];
        TopImage.origin.y = OffsetY - DebugDelta;
        
        // Set positions
        [EditorView setFrame:TextFieldFrame];
        [DebugView setFrame:DebugFrame];
        [TopDiv setFrame:TopImage];
    }
}

@end
