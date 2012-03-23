/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#import "ConsoleTextView.h"

@implementation ConsoleTextView

- (id)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self)
    {
        // Turn off the default rendering of the text
        [self setTextColor:[UIColor colorWithRed:0 green:0 blue:0 alpha:0]];
        
        // Allocate console list
        ConsoleText = [[NSMutableArray alloc] init];
        InputQueue = [[NSMutableArray alloc] init];
    }
    return self;
}

- (void)drawRect:(CGRect)rect
{
    /*** Form Colored String ***/
    
    // Generate the string to append
    NSMutableString* TotalString = [[NSMutableString alloc] init];
    for(NSArray* Pair in ConsoleText)
        [TotalString appendString:[Pair objectAtIndex:1]];
    
    // Create a renderable string with subsections of styles
	ColoredString = [[NSMutableAttributedString alloc] initWithString:TotalString];
    NSRange StringRange = NSMakeRange(0, [TotalString length]);
    
    // Create a font and set it to all text
    CTFontRef MyFont = CTFontCreateWithName((__bridge CFStringRef)[[self font] fontName], [[self font] pointSize], NULL);
    CTFontRef MyBoldFont = CTFontCreateCopyWithSymbolicTraits(MyFont, 0.0, NULL, kCTFontBoldTrait, kCTFontBoldTrait);
	[ColoredString addAttribute:(id)kCTFontAttributeName value:(__bridge id)MyFont range:StringRange];
    
    // Set line spacing
    CGFloat lineSpacing = [[self font] lineHeight];
    CTParagraphStyleSetting theSettings[2] = {
        { kCTParagraphStyleSpecifierMinimumLineHeight, sizeof(CGFloat), &lineSpacing },
        { kCTParagraphStyleSpecifierMaximumLineHeight, sizeof(CGFloat), &lineSpacing },
    };
    
    CTParagraphStyleRef theParagraphRef = CTParagraphStyleCreate(theSettings, 2);
    [ColoredString addAttribute:(id)kCTParagraphStyleAttributeName value:(__bridge id)theParagraphRef range:StringRange];
    
	// Add bold for each output type
    int Offset = 0;
    for(NSArray* Pair in ConsoleText)
    {
        // String length
        int StringLength = [[Pair objectAtIndex:1] length];
        
        // Is it bold?
        if([[Pair objectAtIndex:0] boolValue])
            [ColoredString addAttribute:(id)kCTFontAttributeName value:(__bridge id)MyBoldFont range:NSMakeRange(Offset, StringLength)];
        
        // Add to offset
        Offset += StringLength;
    }
    
    /*** Draw text ***/
    
    // Based on:
    // http://www.cocoanetics.com/2011/01/befriending-core-text/
    
    // Initialize the graphics context
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    // Default color to black
    CGColorRef activeColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:1].CGColor;
    CGContextSetFillColorWithColor(context, activeColor);
    
    // Set font to default font, and retrieve font information
    CGContextSelectFont(context, [[[self font] fontName] UTF8String], [[self font] pointSize], kCGEncodingMacRoman);
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    
    // Flip character matrix, so characters are now right-way up
    CGAffineTransform xform = CGAffineTransformMake(1.0, 0.0, 0.0, -1.0, 0.0, 0.0);
    CGContextSetTextMatrix(context, xform);
    
    // Content height
    float ContentHeight = MAX(self.bounds.size.height, self.contentSize.height);
    
	// Flip the coordinate system for the entire view
	CGContextSetTextMatrix(context, CGAffineTransformIdentity);
	CGContextTranslateCTM(context, 0, ContentHeight);
	CGContextScaleCTM(context, 1.0, -1.0);
    CGContextTranslateCTM(context, 8, -8);
    
    // Start with a layout master
    CGMutablePathRef path = CGPathCreateMutable();
    CGPathAddRect(path, NULL, CGRectMake(0, 0, [self frame].size.width, ContentHeight));
    CTFramesetterRef framesetter = CTFramesetterCreateWithAttributedString((__bridge CFAttributedStringRef)ColoredString);
    CTFrameRef stringFrame = CTFramesetterCreateFrame(framesetter, CFRangeMake(0, [ColoredString length]), path, nil);
    CTFrameDraw(stringFrame, context);
    
    // Release path
    CFRelease(path);
    
    // Done with the graphics work
    UIGraphicsEndImageContext();
}

-(void)pushMessage:(NSString*) Message isOutput:(bool)IsOutput
{
    // Save text
    [ConsoleText addObject:[NSArray arrayWithObjects:[NSNumber numberWithBool:IsOutput], Message, nil]];
    
    // If user input, save to queue
    if(!IsOutput)
        [InputQueue addObject:Message];
    
    // Grow the on-screen text
    [self setText:[NSString stringWithFormat:@"%@%@", [self text], Message]];
    
    // Scroll down
    [self scrollRangeToVisible:NSMakeRange([[self text] length], 0)];
    
    // Force-render new content
    [self setNeedsDisplay];
    [self flashScrollIndicators];
}

-(NSString*) getMessage
{
    // Return nil if no objects
    NSString* QueueTop = nil;
    if([InputQueue count] > 0)
    {
        QueueTop = [InputQueue objectAtIndex:0];
        [InputQueue removeObjectAtIndex:0];
    }
    return QueueTop;
}

@end
