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
    //audioPlayThrough = NULL;
    return self;
}

-(OSStatus) create:(CFStringRef) input :(CFStringRef) output {
    
    if (audioPlayThrough == NULL)
    {
        audioPlayThrough =  new AudioPlayThrough();
    }
    
    return audioPlayThrough->create(input, output);
}

-(OSStatus) start {
    
    if (audioPlayThrough == NULL)
    {
        audioPlayThrough =  new AudioPlayThrough();
    }
    return audioPlayThrough->start();
}

-(OSStatus) stop {
    
    if (audioPlayThrough == NULL)
    {
        audioPlayThrough =  new AudioPlayThrough();
    }
    return audioPlayThrough->stop();
}

-(void) setIsMono:(Boolean) isMono {
    
    if (audioPlayThrough == NULL)
    {
        audioPlayThrough =  new AudioPlayThrough();
    }
    audioPlayThrough->monoInput = isMono;
}

-(void) setAudioUnit:(AudioUnit) audioUnit {
    if (audioPlayThrough == NULL)
    {
        audioPlayThrough =  new AudioPlayThrough();
    }
    
    audioPlayThrough->setAudioUnit(audioUnit);
}

-(void) bypassAudioUnit:(bool) value {
    if (audioPlayThrough == NULL)
    {
        audioPlayThrough =  new AudioPlayThrough();
    }
    audioPlayThrough->bypassAudioUnit(value);
}

- (void)setPeakCallback:(void(*)(Float32 peak))peakCallback {
    if (audioPlayThrough == NULL)
    {
        audioPlayThrough =  new AudioPlayThrough();
    }
    audioPlayThrough->peakCallback = peakCallback;
};

-(Boolean)isRunning {
    audioPlayThrough->isRunning();
}

@end
