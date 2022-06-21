//
//  AudioPlayThroughObjC.h
//  
//
//  Created by Devin Roth on 2022-06-13.
//

#ifndef AudioPlayThroughObjC_h
#define AudioPlayThroughObjC_h

#import <AudioToolbox/AudioToolbox.h>

@interface AudioPlayThroughObjC:NSObject

-(OSStatus) create:(CFStringRef) input :(CFStringRef) output;
-(OSStatus) start;
-(OSStatus) stop;
-(void) setIsMono:(Boolean) isMono;

@end



#endif /* AudioPlayThroughObjC_h */
