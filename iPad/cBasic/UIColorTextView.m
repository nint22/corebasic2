/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#import "UIColorTextView.h"
#import <CoreText/CoreText.h>

@implementation UIColorTextView

- (id)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    if (self)
    {
        // Turn off the default rendering of the text
        [self setTextColor:[UIColor colorWithRed:0 green:0 blue:0 alpha:0]];
        
        // Default the syntax coloring to none
        cbList_Init(&TokenColors);
        ColoredString = nil;
    }
    return self;
}

- (void)textViewDidChange:(UITextView *)textView
{
    /*** Color Highlight ***/
    
    // Release the previous token list
    while(cbList_GetCount(&TokenColors) > 0)
        free(cbList_PopBack(&TokenColors));
    
    // Release the old structure
    cbList_Release(&TokenColors);
    
    // Convert the source code into syntax highlighted code
    TokenColors = cbHighlightCode([[self text] UTF8String]);
    
    /*** Form NSString ***/
    
    // Create a renderable string with subsections of styles
	ColoredString = [[NSMutableAttributedString alloc] initWithString:[self text]];
    NSRange StringRange = NSMakeRange(0, [[self text] length]);
    
    // Create a font and set it to all text
    CTFontRef MyFont = CTFontCreateWithName((__bridge CFStringRef)[[self font] fontName], [[self font] pointSize], NULL);
	[ColoredString addAttribute:(id)kCTFontAttributeName value:(__bridge id)MyFont range:StringRange];
    
    // Set line spacing
    CGFloat lineSpacing = [[self font] lineHeight];
    CTParagraphStyleSetting theSettings[2] = {
        { kCTParagraphStyleSpecifierMinimumLineHeight, sizeof(CGFloat), &lineSpacing },
        { kCTParagraphStyleSpecifierMaximumLineHeight, sizeof(CGFloat), &lineSpacing },
    };
    
    CTParagraphStyleRef theParagraphRef = CTParagraphStyleCreate(theSettings, 2);
    [ColoredString addAttribute:(id)kCTParagraphStyleAttributeName value:(__bridge id)theParagraphRef range:StringRange];
    
	// Add colors for each type
    while(cbList_GetCount(&TokenColors) > 0)
    {
        // Get the colored token element off the list
        cbHighlightToken* Token = cbList_PopBack(&TokenColors);
        
        // Get the color type
        CGColorRef TokenColor = nil;
        if(Token->TokenType == cbTokenType_Comment)
            TokenColor = [UIColor colorWithRed:0 green:0.55f blue:0 alpha:1].CGColor;
        else if(Token->TokenType == cbTokenType_Variable)
            TokenColor = [UIColor blackColor].CGColor;
        else if(Token->TokenType == cbTokenType_Function)
            TokenColor = [UIColor colorWithRed:65.0f/255.0f green:130.0f/255.0f blue:137.0f/255.0f alpha:1].CGColor;
        else if(Token->TokenType == cbTokenType_StringLit)
            TokenColor = [UIColor redColor].CGColor;
        else if(Token->TokenType == cbTokenType_NumericalLit)
            TokenColor = [UIColor blueColor].CGColor;
        else if(Token->TokenType == cbTokenType_Keyword)
            TokenColor = [UIColor colorWithRed:219.0f/255.0f green:46.0f/255.0f blue:163.0f/255.0f alpha:1].CGColor;
        
        // Apply this token's color
        [ColoredString addAttribute:(id)kCTForegroundColorAttributeName value:(__bridge id)TokenColor range:NSMakeRange(Token->Start, Token->Length)];
        
        // Release token
        free(Token);
    }
}

- (void)drawRect:(CGRect)rect
{
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
    CGContextTranslateCTM(context, 8, -8); // This offset is the default offset for regular text rendering
    
    // Start with a layout master
    CGMutablePathRef path = CGPathCreateMutable();
    CGPathAddRect(path, NULL, CGRectMake(0, 0, [self frame].size.width, ContentHeight));
    CTFramesetterRef framesetter = CTFramesetterCreateWithAttributedString((__bridge CFAttributedStringRef)ColoredString);
    CTFrameRef stringFrame = CTFramesetterCreateFrame(framesetter, CFRangeMake(0, [[self text] length]), path, nil);
    CTFrameDraw(stringFrame, context);
    
    // Release path
    CFRelease(path);
    
    // Done with the graphics work
    UIGraphicsEndImageContext();
}

@end
