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

void AudioPlayThroughDestroy(void* audioPlayThrough)
{
    if (audioPlayThrough)
    {
        delete (AudioPlayThrough*)audioPlayThrough;
    }
}

OSStatus AudioPlayThroughStart(void* audioPlayThrough)
{
    if (audioPlayThrough)
    {
        return ((AudioPlayThrough*)audioPlayThrough)->start();
    }

    return 0;
};

OSStatus AudioPlayThroughStop(void* audioPlayThrough)
{
    if (audioPlayThrough)
    {
        return ((AudioPlayThrough*)audioPlayThrough)->stop();
    }

    
    return 0;
};

void AudioPlayThroughSetPeakCallback(void* audioPlayThrough, void(*peakCallback)(Float32 peak))
{
    if (audioPlayThrough)
    {
        ((AudioPlayThrough*)audioPlayThrough)->peakCallback = peakCallback;
    }
    
};

Float32 AudioPlayThroughGetPeak(void* audioPlayThrough)
{
    if (audioPlayThrough)
    {
        return ((AudioPlayThrough*)audioPlayThrough)->getPeak();
    }
    
    return 0;
};

OSStatus AudioPlayThroughSetMatrixLevel(void* audioPlayThrough, UInt32 inputChannel, UInt32 outputChannel, Float32 level)
{
    if (audioPlayThrough)
    {
        return ((AudioPlayThrough*)audioPlayThrough)->setMatrixLevel(inputChannel, outputChannel, level);
    }
    
    return 0;
};

void AudioPlayThroughSetAudioUnit(void* audioPlayThrough, AudioUnit audioUnit)
{
    if (audioPlayThrough)
    {
        ((AudioPlayThrough*)audioPlayThrough)->setAudioUnit(audioUnit);
    }

    
}

void AudioPlayThroughBypassAudioUnit(void* audioPlayThrough, UInt32 value)
{
    if (audioPlayThrough)
    {
        ((AudioPlayThrough*)audioPlayThrough)->bypassAudioUnit(value);
    }
}


const char* GetBuildDate(){
   return __DATE__;
}
