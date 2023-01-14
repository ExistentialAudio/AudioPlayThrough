//
//  AudioPlayThroughC.c
//  
//
//  Created by Devin Roth on 2023-01-13.
//

#include "AudioPlayThroughC.h"
#include "AudioPlayThrough.hpp"

AudioPlayThrough audioPlayThrough = AudioPlayThrough();

void AudioPlayThroughCreate(CFStringRef inputUID, CFStringRef outputUID)
{
    audioPlayThrough.create(inputUID, outputUID);
};

void AudioPlayThroughSetAudioUnit(AudioUnit audioUnit)
{
    audioPlayThrough.setAudioUnit(audioUnit);
};

OSStatus AudioPlayThroughStart(void)
{
    return audioPlayThrough.start();
};

OSStatus AudioPlayThroughStop(void)
{
    return audioPlayThrough.stop();
};
