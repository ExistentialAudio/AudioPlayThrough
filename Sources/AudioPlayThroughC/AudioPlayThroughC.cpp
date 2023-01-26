//
//  AudioPlayThroughC.c
//  
//
//  Created by Devin Roth on 2023-01-13.
//

#include "AudioPlayThroughC.h"
#include "AudioPlayThrough.hpp"

void* AudioPlayThroughCreate(CFStringRef inputUID, CFStringRef outputUID)
{
    AudioPlayThrough* audioPlayThrough = new AudioPlayThrough();
    audioPlayThrough->create(inputUID, outputUID);
    return audioPlayThrough;
    
};

OSStatus AudioPlayThroughStart(void* audioPlayThrough)
{
    return ((AudioPlayThrough*)audioPlayThrough)->start();
};

OSStatus AudioPlayThroughStop(void* audioPlayThrough)
{
    return ((AudioPlayThrough*)audioPlayThrough)->stop();
};

void AudioPlayThroughSetPeakCallback(void* audioPlayThrough, void(*peakCallback)(Float32 peak))
{
    ((AudioPlayThrough*)audioPlayThrough)->peakCallback = peakCallback;
};

Float32 AudioPlayThroughGetPeak(void* audioPlayThrough)
{
    return ((AudioPlayThrough*)audioPlayThrough)->getPeak();
};
