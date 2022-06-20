//
//  AudioPlayThroughObjC.m
//  
//
//  Created by Devin Roth on 2022-06-13.
//

#import <Foundation/Foundation.h>
#import "AudioPlayThroughObjC.h"
#import "AudioPlayThrough.hpp"


@implementation AudioPlayThroughObjC {
    AudioPlayThrough* audioPlayThrough;
}

-(id)init {
    self = [super init];
    audioPlayThrough =  new AudioPlayThrough();
    return self;
}

-(OSStatus) create:(CFStringRef) input :(CFStringRef) output {
    return audioPlayThrough->create(input, output);
}

-(OSStatus) start {
    return audioPlayThrough->start();
}

-(OSStatus) stop {
    return audioPlayThrough->stop();
}

@end
