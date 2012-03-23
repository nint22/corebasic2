//
//  ContextMenu.m
//  cBasic
//
//  Created by Jeremy Bridon on 3/22/12.
//  Copyright (c) 2012 Core S2 Software Solutions. All rights reserved.
//

#import "ContextMenu.h"

@implementation ContextMenu

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self) {
        // Initialization code
    }
    return self;
}

// Only override drawRect: if you perform custom drawing.
// An empty implementation adversely affects performance during animation.
- (void)drawRect:(CGRect)rect
{
    // Initialize the graphics context
    CGContextRef context = UIGraphicsGetCurrentContext();
    
    // Default color to alpha-black
    CGColorRef activeColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:0.5f].CGColor;
    CGContextSetFillColorWithColor(context, activeColor);
    
    // Black line stroke
    CGContextSetLineWidth(context, 2.0);
    CGContextSetStrokeColorWithColor(context, [UIColor blackColor].CGColor);
    
    // Draw elipse with edge border
    CGRect rectangle = CGRectMake(rect.origin.x, rect.origin.y, rect.size.height, rect.size.height);
    CGContextAddEllipseInRect(context, rectangle);
    CGContextFillEllipseInRect(context, rectangle);
    
    // Done with the graphics work
    UIGraphicsEndImageContext();
}

@end
