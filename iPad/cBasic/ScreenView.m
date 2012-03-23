/***************************************************************
 
 cBasic - coreBasic - BASIC Language Interpreter on iOS
 Copyright 2011 Jeremy Bridon - See License.txt for info
 
 This source file is developed and maintained by:
 + Jeremy Bridon jbridon@cores2.com
 
***************************************************************/

#import "ScreenView.h"

@implementation ScreenView

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if(self)
    {
        // Allocate the screen buffer and clear it
        ScreenBuffer = malloc(sizeof(char) * ScreenView_ScreenWidth * ScreenView_ScreenHeight);
        [self clearScreen];
    }
    return self;
}

- (void) dealloc
{
    free(ScreenBuffer);
}

-(void) setPixel: (ScreenView_Color)Color atX: (int)x atY: (int) y
{
    // Ignore if out of bounds
    if(x >= 0 && y >= 0 && x < ScreenView_ScreenWidth && y < ScreenView_ScreenHeight)
        ScreenBuffer[y * ScreenView_ScreenWidth + x] = (char)Color;
    [self setNeedsDisplay];
}

-(void) clearScreen
{
    // Clear the screen (i.e. replace all bytes to 0)
    memset(ScreenBuffer, 0, sizeof(char) * ScreenView_ScreenWidth * ScreenView_ScreenHeight);
    [self setNeedsDisplay];
}

// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect
{
    // Pixel width and height
    float PixelWidth = [self frame].size.width / (float)ScreenView_ScreenWidth;
    float PixelHeight = [self frame].size.height / (float)ScreenView_ScreenHeight;
    
    // Generate the four colors (background, then 3 levels of grayscale
    CGColorRef Colors[4] = {
        [UIColor colorWithRed:0.85f green:0.85f blue:0.85f alpha:1].CGColor,
        [UIColor colorWithRed:0.57f green:0.57f blue:0.57f alpha:1].CGColor,
        [UIColor colorWithRed:0.28f green:0.28f blue:0.28f alpha:1].CGColor,
        [UIColor colorWithRed:0 green:0 blue:0 alpha:1].CGColor,
    };
    
    // Initialize the graphics context
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    // For each pixel
    for(int y = 0; y < ScreenView_ScreenHeight; y++)
    for(int x = 0; x < ScreenView_ScreenWidth; x++)
    {
        // Randomize the color
        // Mod 4 is just so we cycle through colors if needed
        CGContextSetFillColorWithColor(context, Colors[ScreenBuffer[y * ScreenView_ScreenWidth + x] % 4]);
        
        // Draw pixel
        CGContextFillRect(context, CGRectMake((float)x * PixelWidth, (float)y * PixelHeight, PixelWidth, PixelHeight));
    }
    
    // Draw an empty black rectnagle on the border
    CGContextSetStrokeColorWithColor(context, [UIColor blackColor].CGColor);
    CGContextStrokeRect(context, CGRectMake(0, 0, [self frame].size.width, [self frame].size.height));
    
    // Done rendering
    UIGraphicsEndImageContext();
}

@end
