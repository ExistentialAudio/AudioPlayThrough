/*
 
AudioPlayThrough.hpp
 
Copyright (c) 2021 Devin Roth

 */

#ifndef AudioPlayThrough_hpp
#define AudioPlayThrough_hpp



#include <iostream>
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <RequestMicrophoneAuthorization.h>
#include <Accelerate/Accelerate.h>
#include <sys/syslog.h>

#define    DebugMsg(inFormat, ...) \
if (AudioPlayThrough::shouldPrintToOSLog) \
{ \
    syslog(LOG_NOTICE, "AudioPlayThrough:" inFormat " line: %d \n", \
        ## __VA_ARGS__, \
        __LINE__\
        );\
};

#define checkStatus(status) \
if(status && AudioPlayThrough::shouldPrintToOSLog) {\
    OSStatus error = static_cast<OSStatus>(status);\
        syslog(LOG_NOTICE, "AudioPlayThrough Error: %X line: %d\n",  error,\
            __LINE__\
            );\
        return status; \
};


class AudioPlayThrough {
    
    AudioComponentDescription audioComponentDescription;
    
    AudioComponentDescription halAudioComponentDescription = {
        .componentType = kAudioUnitType_Output,
        .componentSubType = kAudioUnitSubType_HALOutput,
        .componentManufacturer = kAudioUnitManufacturer_Apple,
        .componentFlags = 0,
        .componentFlagsMask = 0};
    
    AudioComponentDescription varispeedAudioComponentDescription = {
        .componentType = kAudioUnitType_FormatConverter,
        .componentSubType = kAudioUnitSubType_Varispeed,
        .componentManufacturer = kAudioUnitManufacturer_Apple,
        .componentFlags = 0,
        .componentFlagsMask = 0};
    
    AudioComponentDescription matrixMixerAudioComponentDescription = {
        .componentType = kAudioUnitType_Mixer,
        .componentSubType = kAudioUnitSubType_MatrixMixer,
        .componentManufacturer = kAudioUnitManufacturer_Apple,
        .componentFlags = 0,
        .componentFlagsMask = 0};
    
    AudioComponentDescription newTimePitchAudioComponentDescription = {
        .componentType = kAudioUnitType_FormatConverter,
        .componentSubType = kAudioUnitSubType_NewTimePitch,
        .componentManufacturer = kAudioUnitManufacturer_Apple,
        .componentFlags = 0,
        .componentFlagsMask = 0};
    
    AudioUnit inputAudioUnit = NULL;
    AudioUnit varispeedAudioUnit = NULL;
    AudioUnit audioUnit = NULL;
    AudioUnit multiChannelMixerAudioUnit = NULL;
    AudioUnit newTimePitchAudioUnit = NULL;
    AudioUnit outputAudioUnit = NULL;
    
    AudioDeviceID inputAudioDeviceID = 0;
    AudioDeviceID outputAudioDeviceID = 0;
    
    CFStringRef inputAudioDeviceUID = NULL;
    CFStringRef outputAudioDeviceUID = NULL;
    
    AudioBufferList* inputBuffer = NULL;
    UInt32 ringBufferFrameSize = 192000;
    Float32* ringBuffer = NULL;
    
    Float64 firstInputTime = 0;
    Float64 firstOutputTime = 0;
    Float64 inToOutSampleOffset = 0;
    
    AudioStreamBasicDescription inputAudioStreamBasicDescription;
    AudioStreamBasicDescription outputAudioStreamBasicDescription;
    
    Boolean monoOutput = false;
    
    Float64 inputFrameSize = 0;
    Float64 outputFrameSize = 0;
    
    Boolean _isRunning = false;
    
    dispatch_queue_t queue;
    char queueName[100];
    
    static Boolean shouldPrintToOSLog;
    
    Float32 peak;
    
public:
    AudioPlayThrough();
    OSStatus create(CFStringRef input, CFStringRef output);
    OSStatus start();
    OSStatus stop();
    void setAudioUnit(AudioUnit audioUnit);
    void bypassAudioUnit(UInt32 value);
    ~AudioPlayThrough();
    
    Float32 getPeak() {
        return peak;
    };
    
    Boolean monoInput = true;
    
    void (*peakCallback)(Float32 peak) = NULL;
    
    Boolean isRunning() {
        return _isRunning;
    }
    
    OSStatus setMatrixLevel(UInt32 inputChannel, UInt32 outputChannel, Float32 level);
    
private:
    OSStatus setup();
    
    AudioDeviceID getAudioDeviceID(CFStringRef deviceUID);
    
    OSStatus instantiateAudioUnit(AudioUnit audioUnit, AudioComponentDescription audioComponentDescription);
    OSStatus setupAudioFormats();
    OSStatus setupInput(AudioDeviceID audioDeviceID);
    OSStatus setupVarispeed();
    OSStatus setupMultiChannelMixer();
    OSStatus setupAudioUnit();
    OSStatus setupOutput(AudioDeviceID audioDeviceID);
    OSStatus setupBuffers();
    OSStatus setupConnections();
    OSStatus initializeAudioUnits();
    OSStatus addInputListener();
    OSStatus addOutputListener();
    
    OSStatus takedown();
    OSStatus removeInputListener();
    OSStatus removeOutputListener();
    
    OSStatus addDeviceIsAliveListener(AudioDeviceID audioDeviceID);
    OSStatus removeDeviceIsAliveListener(AudioDeviceID audioDeviceID);
    static OSStatus deviceIsAliveListenerProc(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses, void *inClientData);
    
    Float64 readLocation = 0;
    Float64 writeLocation = 0;
    
    static OSStatus inputProc(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
    
    static OSStatus outputProc(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
    
    static OSStatus streamListenerProc(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses, void *inClientData);
    
    static void changeRate(Float32 rate, void* inRefCon);
    

};


#endif /* PlayThrough_hpp */
