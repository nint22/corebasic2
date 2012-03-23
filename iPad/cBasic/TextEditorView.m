/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#import "TextEditorView.h"
#import "EditorViewController.h"

@implementation TextEditorView

@synthesize TextField, LineColumnView, InstructionsScrollView;

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
    {
        // Initialize the undo / redo stack
        UndoStack = [[NSMutableArray alloc] init];
        RedoStack = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)initialize: (EditorViewController*) Parent
{
    // Save parent to update later
    ParentController = Parent;
    
    // Initialize our column for lines of code
    static const float LineTextOffset = 8;
    LineColumn = [[LineCountViewController alloc] initWithStyle:UITableViewStylePlain];
    [[LineColumn view] setFrame:CGRectMake(0, LineTextOffset, [LineColumnView frame].size.width, [LineColumnView frame].size.height - LineTextOffset)];
    [LineColumnView addSubview:[LineColumn view]];
    
    // Set some sample color-code
    [TextField setDelegate:self];
    
    // Make sure we catch the show / hide of the keyboard
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
    [nc addObserver:self selector:@selector(keyboardWillShow:) name: UIKeyboardWillShowNotification object: nil];
    [nc addObserver:self selector:@selector(keyboardWillHide:) name: UIKeyboardWillHideNotification object: nil];
    
    // Initialize memory list to empty
    InstructionLines = [NSArray arrayWithObjects:@"No program yet loaded in memory...", nil];
    
    // Alloc and initialize our graphical memory output
    InstructionsTable = [[UITableViewController alloc] initWithStyle:UITableViewStylePlain];
    [[InstructionsTable tableView] setDelegate:self];
    [[InstructionsTable tableView] setDataSource:self];
    [[InstructionsTable tableView] setUserInteractionEnabled:false];
    
    // Reload / initialize empty data
    [InstructionsScrollView addSubview:[InstructionsTable tableView]];
    [[InstructionsTable tableView] reloadData];
    
    // Reset odd positioning
    CGRect TableViewFrame = [[InstructionsTable tableView] frame];
    [[InstructionsTable tableView] setFrame:CGRectMake(0, 0, TableViewFrame.size.width, TableViewFrame.size.height)];
    
    // Allow user to drag bottom view
    [[LineColumn view] setUserInteractionEnabled:true];
    UILongPressGestureRecognizer* Gesture = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(debugDragging:)];
    [[LineColumn view] addGestureRecognizer:Gesture];
    
    // Allow key-word context menu view
    UILongPressGestureRecognizer* MenuGesture = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(menuLongPress:)];
    [[self TextField] addGestureRecognizer:MenuGesture];
    
    // Catch all touch gestures on the line count to place / remove debug points
    //UITapGestureRecognizer* TapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(debugTap:)];
    //[TextLineCount addGestureRecognizer:TapGesture];
}

- (bool)Undo
{
    // Can we remove anything? We must always have our original text
    if([UndoStack count] <= 1)
        return false;
    
    // Remove the top of the stack (the last element)
    NSString* PrevText = [UndoStack lastObject];
    [UndoStack removeLastObject];
    
    [RedoStack addObject:PrevText];
    [TextField setText:[UndoStack lastObject]];
    return true;
}

- (bool)Redo
{
    // Can we redo anything?
    if([RedoStack count] <= 0)
        return false;
    
    // Remove the top of the stack (the last element)
    NSString* NextText = [RedoStack lastObject];
    [RedoStack removeLastObject];
    
    [UndoStack addObject:NextText];
    [TextField setText:NextText];
    return true;
}

- (void)setText:(NSString*)Text
{
    // Reset the undo / redo buffers
    [UndoStack removeAllObjects];
    [RedoStack removeAllObjects];
    
    // Make sure to keep the original state in the undo stack
    [UndoStack addObject:Text];
    
    // Set UIColorText content
    [TextField setText:Text];
}

- (void)SetInstructionsSource:(const char*)string
{
    // Turn into manages string to split
    NSString* DataString = [NSString stringWithCString:string encoding:NSASCIIStringEncoding];
    InstructionLines = [DataString componentsSeparatedByString:@"\n"];
    [[InstructionsTable tableView] reloadData];
    
    // Change the frame so that the content can handle it
    [[InstructionsTable tableView] setFrame:CGRectMake(0, 0, 300, [InstructionLines count] * 16.0f)];
    [InstructionsScrollView setContentSize:CGSizeMake(300, [InstructionLines count] * 16.0f)];
}

-(void) keyboardWillShow:(NSNotification *) note
{
    [[note.userInfo valueForKey:UIKeyboardFrameEndUserInfoKey] getValue: &KeyboardRect];
    [ParentController keyboardWillShow:KeyboardRect];
}

-(void) keyboardWillHide:(NSNotification *) note
{
    [[note.userInfo valueForKey:UIKeyboardFrameEndUserInfoKey] getValue: &KeyboardRect];
    [ParentController keyboardWillHide:KeyboardRect];
}

- (void)textViewDidChange:(UITextView *)textView;
{
    // Force-render the text buffer
    [TextField textViewDidChange:textView];
    [TextField setNeedsDisplay];
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    // Force-render the text buffer
    [TextField setNeedsDisplay];
    
    // Keep the line-count field scroll-updated
    [[LineColumn tableView] setContentOffset:[scrollView contentOffset] animated:false];
}

- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text
{
    // Return state
    bool NoChange = true;
    
    // If tabbing, replace with two spaces
    if([text length] == 1 && [text UTF8String][0] == '\t')
    {
        // Where are we placing the new text
        NSString* start = [[textView text] substringWithRange:NSMakeRange(0, range.location)];
        NSString* end = [[textView text] substringFromIndex:range.location];
        
        // Make change
        [textView setText:[NSString stringWithFormat:@"%@  %@", start, end]];
        
        // Place cursor at the end of where we changed (len(start) + 2)
        [textView setSelectedRange:NSMakeRange([start length] + 2, 0)];
        
        // There was custom change
        NoChange = false;
    }
    
    // New line either shifts into a new block, shifts back the previous text, or does both
    else if([text length] == 1 && [text UTF8String][0] == '\n')
    {
        // Total number of white spaces for the new line
        int BlockOffset = 0;
        
        // Where are we placing the new text
        NSString* start = [[textView text] substringWithRange:NSMakeRange(0, range.location)];
        NSString* end = [[textView text] substringFromIndex:range.location];
        
        // Look at the previous line
        NSArray* Lines = [start componentsSeparatedByString:@"\n"];
        NSString* LastLine = [Lines lastObject];
        
        // How many white spaces did the last line have?
        for(int i = 0; i < [LastLine length]; i++)
        {
            if([LastLine characterAtIndex:i] == ' ')
                BlockOffset++;
            else
                break;
        }
        
        // Ignore white spaces
        NSString* LastLineToken = [LastLine stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
        
        // Starts with new block; Grow block offset
        if([LastLineToken hasPrefix:@"while"] || [LastLineToken hasPrefix:@"for"] || [LastLineToken hasPrefix:@"if"] || [LastLineToken hasPrefix:@"elif"] || [LastLineToken hasPrefix:@"else"])
            BlockOffset += 2;
        
        // Create the corrected text
        NSMutableString* NewText = [[NSMutableString alloc] init];
        for(int i = 0; i < [Lines count] - 1; i++)
        {
            [NewText appendString:[Lines objectAtIndex:i]];
            [NewText appendString:@"\n"]; // Since it was removed by the "componentsSeparatedByString"
        }
        
        // If we are closing a block, shift back the end keyword, else, leave alone
        if([LastLineToken hasPrefix:@"end"] || [LastLineToken hasPrefix:@"elif"] || [LastLineToken hasPrefix:@"else"])
        {
            // Current line's offset
            int LineOffset = 0;
            for(int i = 0; i < [LastLine length]; i++)
            {
                if([LastLine characterAtIndex:i] == ' ')
                    LineOffset++;
                else
                    break;
            }
            
            // Last line's offset (if any)
            int LastLineOffset = 0;
            if([Lines count] >= 2)
            {
                NSString* BeforeLastLine = [Lines objectAtIndex:[Lines count] - 2];
                for(int i = 0; i < [BeforeLastLine length]; i++)
                {
                    if([BeforeLastLine characterAtIndex:i] == ' ')
                        LastLineOffset++;
                    else
                        break;
                }
            }
            
            // Shift end-block line back if and only if the spacing is too large from the last keyword
            int Delta = (LineOffset - LastLineOffset);
            if(Delta < 2)
                Delta = 2;
            
            for(int i = 0; i < Delta; i++)
                if([LastLine characterAtIndex:0] == ' ')
                    LastLine = [LastLine substringFromIndex:1];
            
            // We are shifting our forward spaces
            BlockOffset -= 2;
        }
        
        // If negative, just set back to left-most position
        if(BlockOffset < 0)
            BlockOffset = 0;
        
        // Put the last line into the output stack
        [NewText appendString:LastLine];
        [NewText appendString:@"\n"];
        
        // Else if there is an offset, we are tabbing for a new block
        if(BlockOffset > 0)
        {
            // Append the end at the correct spacing
            NSString* EndSpacing = @"";
            for(int i = 0; i < BlockOffset; i++)
                EndSpacing = [NSString stringWithFormat:@"%@ ", EndSpacing];
            [NewText appendString:EndSpacing];
        }
        
        // Save this modified last-line text
        start = NewText;
        
        // Set the new text output
        [TextField setText:[NSString stringWithFormat:@"%@%@", start, end]];
        
        // Set corrected cursor position (each block is 2 spaces)
        [textView setSelectedRange:NSMakeRange([start length], 0)];
        
        // There was custom change
        NoChange = false;
    }
    
    // If custom text, then save it
    if(!NoChange)
    {
        [UndoStack addObject:[TextField text]];
    }
    // Not custom text, generate it
    else
    {
        NSMutableString* NewText = [[NSMutableString alloc] initWithString:[textView text]];
        [NewText insertString:text atIndex:range.location];
        [UndoStack addObject:NewText];
    }
    
    // Make sure to never let the undo stack get more than 32 elements
    while([UndoStack count] > 32)
        [UndoStack removeObjectAtIndex:0];
    
    // Clear out redo-buffer
    [RedoStack removeAllObjects];
    
    // Return true or false based on if we had any custom changes
    return NoChange;
}

/*** Table View Callbacks ***/

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return [InstructionLines count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell* cell = [tableView dequeueReusableCellWithIdentifier:@""];
    if (cell == nil)
    {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:@""];
        [[cell textLabel] setFont:[UIFont fontWithName:@"CourierNewPS-BoldMT" size:12.0f]];
    }
    
    [[cell textLabel] setText:[InstructionLines objectAtIndex:[indexPath row]]];
    return cell;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return 16.0f;
}

/*** Side-view drag ***/

-(void) debugDragging:(UILongPressGestureRecognizer*)sender
{
    // Drag being held down
    if([sender state] == UIGestureRecognizerStateChanged)
    {
        // Get the max width of the instructions table
        float OffsetX = [sender locationInView:self].x;
        
        // Bounds check
        if(OffsetX < 100)
            OffsetX = 100;
        else if(OffsetX > 300)
            OffsetX = 300;
        
        /** Drag views **/
        
        // TODO
        
        // Left scroll view needs width resize
        CGRect LeftRect = [InstructionsScrollView frame];
        LeftRect.size.width = OffsetX - 1; // Space to render the bounds line of the line count column
        
        // Lines scroll needs position change
        CGRect MiddleRect = [LineColumnView frame];
        MiddleRect.origin.x = OffsetX;
        
        // Right text view needs width resize
        CGRect RightRect = [TextField frame];
        RightRect.origin.x = OffsetX + MiddleRect.size.width + 1;
        RightRect.size.width = [self frame].size.width - LeftRect.size.width - MiddleRect.size.width - 1;
        
        // Set positions
        [InstructionsScrollView setFrame:LeftRect];
        [LineColumnView setFrame:MiddleRect];
        [TextField setFrame:RightRect];
        
        // Need to render
        [self setNeedsDisplay];
    }
}

/*** Context Menu ***/

-(void) menuLongPress:(UILongPressGestureRecognizer*)sender
{
    // Allocate and add the special menu view
    EditorMenu = [[ContextMenu alloc] initWithFrame:CGRectMake(10, 10, 200, 200)];
    [self addSubview:EditorMenu];
}

/*** Breakpoint Installation / Removal ***/

-(void) debugTap:(UITapGestureRecognizer*)sender
{
    // Ignore unless we finished our gesture
    if([sender state] != UIGestureRecognizerStateEnded)
        return;
    /*
    // What line did the user tap?
    CGPoint TouchPt = [sender locationInView:TextLineCount];
    int LineIndex = TouchPt.y / [[TextLineCount font] lineHeight];
    */
    // Place a debug point on said line
}

/*** Special Draw Rect to but a black border on the line object ***/

-(void) drawRect:(CGRect)rect
{
    // Tell the parent to draw normally
    [super drawRect:rect];
    
    // Grab the context
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    // Define the color
    CGContextSetStrokeColorWithColor(context, [UIColor blackColor].CGColor);
    
    // Frame we are working on
    CGRect ColumnFrame = [LineColumnView frame];
    
    // Draw line left
    CGPoint LinePoints[2];
    LinePoints[0].x = ColumnFrame.origin.x; LinePoints[0].y = ColumnFrame.origin.y;
    LinePoints[1].x = ColumnFrame.origin.x; LinePoints[1].y = ColumnFrame.origin.y + [LineColumnView frame].size.height;
    CGContextStrokeLineSegments(context, LinePoints, 2);
    
    // Draw right
    LinePoints[0].x = ColumnFrame.origin.x + ColumnFrame.size.width; LinePoints[0].y = ColumnFrame.origin.y;
    LinePoints[1].x = ColumnFrame.origin.x + ColumnFrame.size.width; LinePoints[1].y = ColumnFrame.origin.y + [LineColumnView frame].size.height;
    CGContextStrokeLineSegments(context, LinePoints, 2);
    
    // Done rendering
    UIGraphicsEndImageContext();
}

@end
