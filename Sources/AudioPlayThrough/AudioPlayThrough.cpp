/*
 
AudioPlayThrough.cpp

Copyright (c) 2021 Devin Roth
 
 */

#include "AudioPlayThrough.h"

#if DEBUG
Boolean AudioPlayThrough::shouldPrintToOSLog = true;
#else
Boolean AudioPlayThrough::shouldPrintToOSLog = true;
#endif

AudioPlayThrough::AudioPlayThrough()
{
    std::snprintf(queueName, 100, "AudioPlayThroughQueue_%d", rand());
    
    queue = dispatch_queue_create(queueName, NULL);
    
    // Instantiate Audio Units
    instantiateAudioUnit(inputAudioUnit, halAudioComponentDescription);
    instantiateAudioUnit(varispeedAudioUnit, varispeedAudioComponentDescription);
    instantiateAudioUnit(newTimePitchAudioUnit, newTimePitchAudioComponentDescription);
    instantiateAudioUnit(multiChannelMixerAudioUnit, matrixMixerAudioComponentDescription);
    instantiateAudioUnit(outputAudioUnit, halAudioComponentDescription);
    
}

OSStatus AudioPlayThrough::instantiateAudioUnit(AudioUnit audioUnit, AudioComponentDescription audioComponentDescription){
    
    AudioComponent audioComponent = AudioComponentFindNext(NULL, &audioComponentDescription);
    
    if (audioComponent == NULL) {
        DebugMsg("Unable to find HALOutput AudioComponent. \n");
        return 1234;
    }

    AudioComponentInstantiate(audioComponent,
                              kAudioComponentInstantiation_LoadOutOfProcess,
                              ^(AudioComponentInstance audioUnit, OSStatus error) {
        if (error) {
            DebugMsg("Unable to instantiate HALOutput AudioComponent. \n");
            abort();
        }
    });
    
    return noErr;
}

OSStatus AudioPlayThrough::create(CFStringRef input, CFStringRef output){
    
    inputAudioDeviceUID = CFStringCreateCopy(NULL, input);;
    outputAudioDeviceUID = CFStringCreateCopy(NULL, output);;
    return noErr;
}

AudioDeviceID AudioPlayThrough::getAudioDeviceID(CFStringRef deviceUID){
    
    // get ID from UID
    AudioObjectPropertyAddress inAddress;
    inAddress.mSelector = kAudioHardwarePropertyTranslateUIDToDevice;
    UInt32 inQualifierDataSize = sizeof(CFStringRef);
    CFStringRef inQualifierData = deviceUID;
    UInt32 ioDataSize = sizeof(AudioObjectID);
    AudioObjectID inDeviceID;

    OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &inAddress, inQualifierDataSize, &inQualifierData, &ioDataSize, &inDeviceID);
    
    if (status)
    {
        DebugMsg("Failed to retrieve AudioDeviceID from UID.");
    }
    
    return inDeviceID;
}

OSStatus AudioPlayThrough::setup(){
    
    checkStatus(setupAudioFormats());
    checkStatus(setupInput(inputAudioDeviceID));
    checkStatus(setupMultiChannelMixer());
    checkStatus(setupVarispeed());
    checkStatus(setupAudioUnit());
    checkStatus(setupOutput(outputAudioDeviceID));
    checkStatus(setupConnections());
    checkStatus(setupBuffers());
    checkStatus(initializeAudioUnits());
    checkStatus(addInputListener());
    checkStatus(addOutputListener());
    checkStatus(addDeviceIsAliveListener(inputAudioDeviceID));
    checkStatus(addDeviceIsAliveListener(outputAudioDeviceID));
    return noErr;
};

OSStatus AudioPlayThrough::start()
{

    
    __block OSStatus status = noErr;
    
    if (queue == NULL) {
        return 1;
    }
    dispatch_sync(queue, ^{
        
        firstInputTime = -1;
        firstOutputTime = -1;
        inToOutSampleOffset = 0;
        
        writeLocation = 0;
        readLocation = 0;
        
        inputAudioDeviceID = getAudioDeviceID(inputAudioDeviceUID);
        
        if (inputAudioDeviceID == 0) {
            DebugMsg("Unable to start AudioPlayThrough. Invalid input device.");
            status = kAudioHardwareBadDeviceError;
            return;
        }
        
        outputAudioDeviceID = getAudioDeviceID(outputAudioDeviceUID);
        
        if (outputAudioDeviceID == 0) {
            DebugMsg("Unable to start AudioPlayThrough. Invalid output device.");
            status = kAudioHardwareBadDeviceError;
            return;
        }
        
        
        setup();

        if (inputAudioUnit == NULL || outputAudioUnit == NULL) {
            DebugMsg("Unable to start AudioPlayThrough. AudioPlayThrough is not setup. \n");
            status = kAudioUnitErr_Initialized;
            return;
        }
        
        status = (AudioOutputUnitStart(inputAudioUnit));
        if (status != noErr)
        {
            return;
        }
        
        status = (AudioOutputUnitStart(outputAudioUnit));
        if (status != noErr)
        {
            return;
        }
        
        DebugMsg("AudioPlayThrough Started.");
        _isRunning = true;
        
    });
    
    return status;
}

OSStatus AudioPlayThrough::stop()
{

    __block OSStatus status = noErr;
    
    if (_isRunning)
    {
        _isRunning = false;
        
        dispatch_sync(queue, ^{
        
            if (inputAudioUnit != NULL) status = (AudioOutputUnitStop(inputAudioUnit));
            if (status != noErr)
            {
                return;
            }
            
            if (inputAudioUnit != NULL) status = (AudioOutputUnitStop(outputAudioUnit));
                if (status != noErr)
            {
                return;
            }
            
        });
        
    }
    
    takedown();

    
    return noErr;
}

AudioPlayThrough::~AudioPlayThrough()
{
    stop();
    takedown();
}

OSStatus AudioPlayThrough::inputProc(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData){
    
    AudioPlayThrough* This = (AudioPlayThrough*)inRefCon;
    
    if (!This->_isRunning){
        return noErr;
    }
    
    This->inputFrameSize = inNumberFrames;
    
    // Get the new audio data
    checkStatus(AudioUnitRender(This->inputAudioUnit,
                         ioActionFlags,
                         inTimeStamp,
                         inBusNumber,
                         inNumberFrames,
                         This->inputBuffer));
    
    // Copy audio to the ringbuffer.
    UInt32 channels = This->inputBuffer->mNumberBuffers;
    
    
    for (UInt32 frame = 0; frame < inNumberFrames; frame ++){
        for (UInt32 channel = 0; channel < channels; channel++){
            Float32* buffer = (Float32*)This->inputBuffer->mBuffers[channel].mData;
            UInt64 ringBufferLocation = ((UInt64)(inTimeStamp->mSampleTime + frame) % This->ringBufferFrameSize) + This->ringBufferFrameSize * channel;
            This->ringBuffer[ringBufferLocation] = buffer[frame];
            
        }
    }
    

    Float32 peak = 0;
    for (UInt32 frame = 0; frame < inNumberFrames; frame ++)
    {
        if (((Float32*)This->inputBuffer->mBuffers[0].mData)[frame] > peak)
        {
            peak = ((Float32*)This->inputBuffer->mBuffers[0].mData)[frame];
        }
    }
    if (peak > 1)
    {
        peak = 1;
    }

    dispatch_async(dispatch_get_main_queue(), ^{
        This->peak = peak;
        
        if (This->_isRunning && This->peakCallback != NULL)
        {
            This->peakCallback(This->peak);
        }
    });
    
    This->writeLocation = inTimeStamp->mSampleTime + inNumberFrames;
    
    return noErr;
}
inline void MakeBufferSilent (AudioBufferList * ioData)
{
    for(UInt32 i=0; i<ioData->mNumberBuffers;i++)
        memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
}

OSStatus AudioPlayThrough::outputProc(void *inRefCon, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData){
    
    AudioPlayThrough* This = (AudioPlayThrough*)inRefCon;
    
    if (!This->_isRunning){
        MakeBufferSilent (ioData);
        return noErr;
    }

    //use the varispeed playback rate to offset small discrepancies in sample rate
    //first find the rate scalars of the input and output devices
    Float64 rate = 0.0;

    UInt64 hostTime = AudioGetCurrentHostTime();
    
    AudioTimeStamp inTime;
    inTime.mFlags = kAudioTimeStampHostTimeValid;
    inTime.mHostTime = hostTime;
    
    AudioTimeStamp inputTime, outputTime;
    inputTime.mFlags = kAudioTimeStampRateScalarValid;
    outputTime.mFlags = kAudioTimeStampRateScalarValid;

    // This will run a few times after stop. It will print a warning.
    OSStatus status;
    status = AudioDeviceTranslateTime(This->inputAudioDeviceID, &inTime, &inputTime);
    if (status){
        MakeBufferSilent (ioData);
        return noErr;
    }
    status = AudioDeviceTranslateTime(This->outputAudioDeviceID, &inTime, &outputTime);
    if (status){
        MakeBufferSilent (ioData);
        return noErr;
    }
    
    rate = inputTime.mRateScalar / outputTime.mRateScalar;
    
    
    This->outputFrameSize = inNumberFrames;
    
    if (This->writeLocation == 0){
        // input hasn't run yet -> silence
        MakeBufferSilent (ioData);
        return noErr;
    }
    
    // Set the initial read location.
    if (This->firstOutputTime == -1){
        This->firstOutputTime = 1;
        // calculate the offset
        This->inToOutSampleOffset = This->writeLocation - This->outputFrameSize - This->inputFrameSize - inTimeStamp->mSampleTime;
        MakeBufferSilent (ioData);
        return  noErr;
    }
    
    
    // set the read location
    This->readLocation = inTimeStamp->mSampleTime + This->inToOutSampleOffset;
    
    if (This->readLocation > This->writeLocation - inNumberFrames) {
        DebugMsg("Trying to read before audio is written. Resetting sync. \n");
        This->firstOutputTime = -1;
        MakeBufferSilent (ioData);
        return  noErr;
    }
    
    if (This->readLocation < This->writeLocation - This->outputFrameSize*2 - This->inputFrameSize*2) {
        DebugMsg("Reading is way behind. Resetting sync. \n");
        This->firstOutputTime = -1;
        MakeBufferSilent (ioData);
        return  noErr;
    }
    
    // Calibrate to lowest possible latency.
    // The theory here is that we can get as close to the write location as possible but never cross it.
    
    // Let's set the rate to be a little fast. That way we can butt up against the write location and slow it down if needed.
    
    //   http://www.cochlea.eu/en/sound/psychoacoustics/pitch
    // I found that not to be true. I can easily hear a variation of 0.002. The goal here is to pick a number low enough that we can't hear it but high enough to compensate for even the worst audio clocks.

    Float64 difference = This->writeLocation - This->readLocation - This->outputFrameSize - This->inputFrameSize;
    

    if (difference < 0) {
        // needs to be slower.
        Float64 scale = 0.000001;
        rate -= scale;
    } else {
        // needs to be faster.
        Float64 scale = 0.000001;
        rate += scale;
    }
    //printf("Difference: %f \t rate: %f \n", difference, rate);

    // set the rate for the varispeed
    checkStatus(AudioUnitSetParameter(This->varispeedAudioUnit,
                                   kVarispeedParam_PlaybackRate,
                                   kAudioUnitScope_Global,
                                   0,
                                   rate,
                                   0));

    // Copy audio from the ringbuffer.
    UInt32 channels = ioData->mNumberBuffers;
    
    for (UInt32 frame = 0; frame < inNumberFrames; frame ++){
        for (UInt32 channel = 0; channel < channels; channel++){
            
            Float32* buffer = (Float32*)ioData->mBuffers[channel].mData;
            UInt64 ringBufferLocation = ((UInt64)(This->readLocation + frame) % This->ringBufferFrameSize) + This->ringBufferFrameSize * channel;
            if (This->ringBuffer != NULL) {
                buffer[frame] = This->ringBuffer[ringBufferLocation];
            }


        }
    }
    
    return noErr;
}

OSStatus AudioPlayThrough::streamListenerProc(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses, void *inClientData){
    
    dispatch_async(dispatch_get_global_queue( QOS_CLASS_BACKGROUND, 0), ^{
        
        AudioPlayThrough* This = (AudioPlayThrough*)inClientData;
        
        if (This->_isRunning)
        {
            This->takedown();
            This->start();
        }
        
    });


    
    return noErr;
}

OSStatus AudioPlayThrough::setupAudioFormats(){
    
    AudioObjectPropertyAddress inAddress;
    inAddress.mSelector = kAudioDevicePropertyStreams;
    inAddress.mScope = kAudioObjectPropertyScopeInput;
    
    AudioDeviceID streamAudioDeviceID;
    UInt32 size = sizeof(AudioDeviceID);
    
    checkStatus(AudioObjectGetPropertyData(inputAudioDeviceID, &inAddress, 0, NULL, &size, &streamAudioDeviceID));
    
    inAddress.mSelector = kAudioDevicePropertyStreamFormat;
    inAddress.mScope = kAudioObjectPropertyScopeGlobal;
    size = sizeof(AudioStreamBasicDescription);
    
    checkStatus(AudioObjectGetPropertyData(streamAudioDeviceID, &inAddress, 0, NULL, &size, &inputAudioStreamBasicDescription));
    
    inAddress.mSelector = kAudioDevicePropertyStreams;
    inAddress.mScope = kAudioObjectPropertyScopeOutput;
    
    size = sizeof(AudioDeviceID);
    
    checkStatus(AudioObjectGetPropertyData(outputAudioDeviceID, &inAddress, 0, NULL, &size, &streamAudioDeviceID));
    
    inAddress.mSelector = kAudioDevicePropertyStreamFormat;
    inAddress.mScope = kAudioObjectPropertyScopeGlobal;
    size = sizeof(AudioStreamBasicDescription);
    
    checkStatus(AudioObjectGetPropertyData(streamAudioDeviceID, &inAddress, 0, NULL, &size, &outputAudioStreamBasicDescription));
    
    // Everything works better if we stick with stereo. Hardcoded to 2 channels.
    //inputAudioStreamBasicDescription.mChannelsPerFrame = 2;
    outputAudioStreamBasicDescription.mChannelsPerFrame = 2;
    
    return noErr;
}

OSStatus AudioPlayThrough::setupInput(AudioDeviceID audioDeviceID){
    
    AudioUnit& audioUnit = inputAudioUnit;
    
    AudioComponent comp;
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;

    //Finds a component that meets the desc spec's
    comp = AudioComponentFindNext(NULL, &desc);
    if (comp == NULL) return -1;

    //gains access to the services provided by the component
    checkStatus(AudioComponentInstanceNew(comp, &audioUnit));

    instantiateAudioUnit(inputAudioUnit, halAudioComponentDescription);
    
    UInt32 enableIO;
    enableIO = 1;
    checkStatus( AudioUnitSetProperty(audioUnit,
                                kAudioOutputUnitProperty_EnableIO,
                                kAudioUnitScope_Input,
                                1, // input element
                                &enableIO,
                                sizeof(enableIO)));
    
    
    enableIO = 0;
    checkStatus(AudioUnitSetProperty(audioUnit,
                              kAudioOutputUnitProperty_EnableIO,
                              kAudioUnitScope_Output,
                              0,   //output element
                              &enableIO,
                              sizeof(enableIO)));
    
    // Set the input device to the AUHAL.
    // this should be done only after IO has been enabled on the AUHAL.
    checkStatus(AudioUnitSetProperty(audioUnit,
                              kAudioOutputUnitProperty_CurrentDevice,
                              kAudioUnitScope_Global,
                              0,
                              &audioDeviceID,
                              sizeof(audioDeviceID)));
    
    
    //Tell the output unit not to reset timestamps
    //Otherwise sample rate changes will cause sync loss
    UInt32 startAtZero = 0;
    checkStatus(AudioUnitSetProperty(audioUnit,
                              kAudioOutputUnitProperty_StartTimestampsAtZero,
                              kAudioUnitScope_Global,
                              0,
                              &startAtZero,
                              sizeof(startAtZero)));
    
    
    //Setup the input callback.
    AURenderCallbackStruct input;
    
    input.inputProc = inputProc;
    input.inputProcRefCon = this;
    
    checkStatus(AudioUnitSetProperty(audioUnit,
                              kAudioOutputUnitProperty_SetInputCallback,
                              kAudioUnitScope_Global,
                              0,
                              &input,
                              sizeof(input)));
    
    // Set the format
    AudioStreamBasicDescription asbd;
    UInt32 propertySize = sizeof(AudioStreamBasicDescription);
    checkStatus(AudioUnitGetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, &propertySize));

    asbd.mSampleRate = inputAudioStreamBasicDescription.mSampleRate;
    asbd.mChannelsPerFrame = inputAudioStreamBasicDescription.mChannelsPerFrame;

    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, propertySize));
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &asbd, propertySize));
    

    
    
    return noErr;
}

OSStatus AudioPlayThrough::setupOutput(AudioDeviceID audioDeviceID){
    
    AudioComponent comp;
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_HALOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    //Finds a component that meets the desc spec's
    comp = AudioComponentFindNext(NULL, &desc);
    if (comp == NULL) return -1;
    
    //gains access to the services provided by the component
    checkStatus(AudioComponentInstanceNew(comp, &outputAudioUnit));
    
    
    //AUHAL needs to be initialized before anything is done to it
    checkStatus(AudioUnitInitialize(outputAudioUnit));
    
    
    // Enable input and disable output.
    UInt32 enableIO;
    enableIO = 0;
    checkStatus( AudioUnitSetProperty(outputAudioUnit,
                                kAudioOutputUnitProperty_EnableIO,
                                kAudioUnitScope_Input,
                                1, // input element
                                &enableIO,
                                sizeof(enableIO)));
    
    
    enableIO = 1;
    checkStatus(AudioUnitSetProperty(outputAudioUnit,
                              kAudioOutputUnitProperty_EnableIO,
                              kAudioUnitScope_Output,
                              0,   //output element
                              &enableIO,
                              sizeof(enableIO)));
    
    // Set the output device to the AUHAL.
    // this should be done only after IO has been enabled on the AUHAL.
    checkStatus(AudioUnitSetProperty(outputAudioUnit,
                              kAudioOutputUnitProperty_CurrentDevice,
                              kAudioUnitScope_Global,
                              0,
                              &audioDeviceID,
                              sizeof(audioDeviceID)));
    
    
    //Tell the output unit not to reset timestamps
    //Otherwise sample rate changes will cause sync loss
    UInt32 startAtZero = 0;
    checkStatus(AudioUnitSetProperty(outputAudioUnit,
                              kAudioOutputUnitProperty_StartTimestampsAtZero,
                              kAudioUnitScope_Global,
                              0,
                              &startAtZero,
                              sizeof(startAtZero)));

    // Set the format
    AudioStreamBasicDescription asbd;
    UInt32 propertySize = sizeof(AudioStreamBasicDescription);
    checkStatus(AudioUnitGetProperty(outputAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, &propertySize));

    asbd.mSampleRate = outputAudioStreamBasicDescription.mSampleRate;
    asbd.mChannelsPerFrame = outputAudioStreamBasicDescription.mChannelsPerFrame;

    checkStatus(AudioUnitSetProperty(outputAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, propertySize));
    checkStatus(AudioUnitSetProperty(outputAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &asbd, propertySize));
    
    UInt32 maxFrames = 4096;
    UInt32 size = sizeof(UInt32);
    checkStatus(AudioUnitSetProperty(outputAudioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Input, 0, &maxFrames, size));
    checkStatus(AudioUnitSetProperty(outputAudioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Output, 1, &maxFrames, size));
    
    
    // Initialize the audio unit.
    checkStatus(AudioUnitInitialize(outputAudioUnit));
    
    
    return noErr;
}

OSStatus AudioPlayThrough::setupMultiChannelMixer(){
    AudioUnit& audioUnit = multiChannelMixerAudioUnit;
    
    // load the audio unit
    AudioComponent comp;
    //Finds a component that meets the desc spec's
    comp = AudioComponentFindNext(NULL, &matrixMixerAudioComponentDescription);
    if (comp == NULL) return -1;
    
    //gains access to the services provided by the component
    checkStatus(AudioComponentInstanceNew(comp, &audioUnit));
    
    //Setup the input callback.
    AURenderCallbackStruct input;
    
    input.inputProc = outputProc;
    input.inputProcRefCon = this;
    
    checkStatus(AudioUnitSetProperty(audioUnit,
                              kAudioUnitProperty_SetRenderCallback,
                              kAudioUnitScope_Input,
                              0,
                              &input,
                              sizeof(input)));
    
    // Set the format
    AudioStreamBasicDescription asbd;
    UInt32 propertySize = sizeof(AudioStreamBasicDescription);
    checkStatus(AudioUnitGetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, &propertySize));

    // Set the input to the input sample rate.
    asbd.mSampleRate = inputAudioStreamBasicDescription.mSampleRate;
    asbd.mChannelsPerFrame = inputAudioStreamBasicDescription.mChannelsPerFrame;
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, propertySize));
    asbd.mChannelsPerFrame = outputAudioStreamBasicDescription.mChannelsPerFrame;
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &asbd, propertySize));
    
    UInt32 maxFrames = 4096;
    UInt32 size = sizeof(UInt32);
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Input, 0, &maxFrames, size));
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Output, 1, &maxFrames, size));
    
    
    UInt32 busCount = 1;
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_BusCount, kAudioUnitScope_Output, 1, &busCount, size));
    
    AudioUnitInitialize(audioUnit);

    UInt32 dimensions[2];
    size = sizeof(dimensions);
    
    checkStatus(AudioUnitGetProperty(audioUnit, kAudioUnitProperty_MatrixDimensions, kAudioUnitScope_Global, 0, &dimensions, &size));
    
    Float32 matrixLevels[dimensions[0]+1][dimensions[1]+1];
    size = (UInt32)sizeof(matrixLevels);
    
    
    // input masters
    for (UInt32 inputChannel = 0; inputChannel < dimensions[0]+1; inputChannel++)
    {
        // output masters
        for (UInt32 outputChannel = 0; outputChannel < dimensions[1]+1; outputChannel++)
        {
            matrixLevels[inputChannel][outputChannel] = 0;
        }
    }
    
    
    
    // input masters
    for (UInt32 inputChannel = 0; inputChannel < dimensions[0]+1; inputChannel++)
    {
        matrixLevels[inputChannel][dimensions[1]] = 1.0;
    }
    
    // output masters
    for (UInt32 outputChannel = 0; outputChannel < dimensions[1]+1; outputChannel++)
    {
        matrixLevels[dimensions[0]][outputChannel] = 1.0;
    }
    
    // master volume
    matrixLevels[dimensions[0]][dimensions[1]] = 1.0;
    
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_MatrixLevels, kAudioUnitScope_Global, 0, &matrixLevels, size));
    
    return noErr;
};

OSStatus AudioPlayThrough::setMatrixLevel(UInt32 inputChannel, UInt32 outputChannel, Float32 level)
{

    UInt32 dimensions[2];
    UInt32 size = sizeof(dimensions);
    
    checkStatus(AudioUnitGetProperty(multiChannelMixerAudioUnit, kAudioUnitProperty_MatrixDimensions, kAudioUnitScope_Global, 0, &dimensions, &size));
    
    Float32 matrixLevels[dimensions[0]+1][dimensions[1]+1];
    size = (UInt32)sizeof(matrixLevels);
    
    checkStatus(AudioUnitGetProperty(multiChannelMixerAudioUnit, kAudioUnitProperty_MatrixLevels, kAudioUnitScope_Global, 0, &matrixLevels, &size));
    
    if (inputChannel > dimensions[0]) { return noErr; }
    if (outputChannel > dimensions[1]) { return noErr; }
    
    matrixLevels[inputChannel][outputChannel] = level;
    
    checkStatus(AudioUnitSetProperty(multiChannelMixerAudioUnit, kAudioUnitProperty_MatrixLevels, kAudioUnitScope_Global, 0, &matrixLevels, size))
    
    return noErr;
}

OSStatus AudioPlayThrough::setupVarispeed(){
    AudioUnit& audioUnit = varispeedAudioUnit;
    
    // load the audio unit
    AudioComponentDescription desc;
    desc.componentType = kAudioUnitType_FormatConverter;
    desc.componentSubType = kAudioUnitSubType_Varispeed;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    desc.componentFlags = 0;
    desc.componentFlagsMask = 0;
    
    AudioComponent comp;
    //Finds a component that meets the desc spec's
    comp = AudioComponentFindNext(NULL, &desc);
    if (comp == NULL) return -1;
    
    //gains access to the services provided by the component
    checkStatus(AudioComponentInstanceNew(comp, &audioUnit));
    
    
//    //Setup the input callback.
//    AURenderCallbackStruct input;
//    
//    input.inputProc = outputProc;
//    input.inputProcRefCon = this;
//    
//    checkStatus(AudioUnitSetProperty(audioUnit,
//                              kAudioUnitProperty_SetRenderCallback,
//                              kAudioUnitScope_Input,
//                              0,
//                              &input,
//                              sizeof(input)));
    
    // Set the format
    AudioStreamBasicDescription asbd;
    UInt32 propertySize = sizeof(AudioStreamBasicDescription);
    checkStatus(AudioUnitGetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, &propertySize));

    // Set the input to the input sample rate.
    asbd.mSampleRate = inputAudioStreamBasicDescription.mSampleRate;
    asbd.mChannelsPerFrame = inputAudioStreamBasicDescription.mChannelsPerFrame;
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, propertySize));
    
    // Set the output to the output sample rate.
    asbd.mSampleRate = outputAudioStreamBasicDescription.mSampleRate;
    asbd.mChannelsPerFrame = outputAudioStreamBasicDescription.mChannelsPerFrame;
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &asbd, propertySize));
    
    UInt32 maxFrames = 4096;
    UInt32 size = sizeof(UInt32);
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Input, 0, &maxFrames, size));
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_MaximumFramesPerSlice, kAudioUnitScope_Output, 1, &maxFrames, size));
    
    return noErr;
};

void AudioPlayThrough::setAudioUnit(AudioUnit audioUnit){
    this->audioUnit = audioUnit;
};

void AudioPlayThrough::bypassAudioUnit(UInt32 value){
    UInt32 propertySize = sizeof(UInt32);
    AudioUnitSetProperty(audioUnit, kAudioUnitProperty_BypassEffect, kAudioUnitScope_Global, 1, &value, propertySize);
};

OSStatus AudioPlayThrough::setupAudioUnit(){
    
    
    if (audioUnit == NULL) {
            
        AudioComponentDescription desc;
        desc.componentType = kAudioUnitType_Effect;
        desc.componentSubType = kAudioUnitSubType_Delay;
        desc.componentManufacturer = kAudioUnitManufacturer_Apple;
        desc.componentFlags = 0;
        desc.componentFlagsMask = 0;
        
        AudioComponent comp;
        //Finds a component that meets the desc spec's
        comp = AudioComponentFindNext(NULL, &desc);
        if (comp == NULL) {
            printf("Could not find audio component.\n");
            return -1;
            
        };
        
        checkStatus(AudioComponentInstanceNew(comp, &audioUnit));
    }
    
    
    AudioUnitUninitialize(audioUnit);
    
    // Connect the audio units together.
    AudioUnitConnection connection;
    connection.sourceAudioUnit = NULL;
    connection.destInputNumber = 0;
    connection.sourceOutputNumber = 0;
    
    checkStatus(AudioUnitSetProperty(audioUnit,
                              kAudioUnitProperty_MakeConnection,
                              kAudioUnitScope_Input,
                              0,
                              &connection,
                              sizeof(connection)));
    
    // Set the format
    AudioStreamBasicDescription asbd;
    UInt32 propertySize = sizeof(AudioStreamBasicDescription);
    checkStatus(AudioUnitGetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, &propertySize));

    // Set the input to the input sample rate.
    asbd.mSampleRate = outputAudioStreamBasicDescription.mSampleRate;
    asbd.mChannelsPerFrame = outputAudioStreamBasicDescription.mChannelsPerFrame;
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &asbd, propertySize));

    // Set the output to the output sample rate.
    asbd.mSampleRate = outputAudioStreamBasicDescription.mSampleRate;
    asbd.mChannelsPerFrame = outputAudioStreamBasicDescription.mChannelsPerFrame;
    checkStatus(AudioUnitSetProperty(audioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &asbd, propertySize));
    
    bypassAudioUnit(1);
    
    return noErr;
};

OSStatus AudioPlayThrough::setupConnections(){
    
    // Connect the audio units together.
    AudioUnitConnection connection;
    connection.sourceAudioUnit = multiChannelMixerAudioUnit;
    connection.destInputNumber = 0;
    connection.sourceOutputNumber = 0;
    
    checkStatus(AudioUnitSetProperty(varispeedAudioUnit,
                              kAudioUnitProperty_MakeConnection,
                              kAudioUnitScope_Input,
                              0,
                              &connection,
                              sizeof(connection)));


    connection.sourceAudioUnit = varispeedAudioUnit;

    checkStatus(AudioUnitSetProperty(audioUnit,
                              kAudioUnitProperty_MakeConnection,
                              kAudioUnitScope_Input,
                              0,
                              &connection,
                              sizeof(connection)));


    connection.sourceAudioUnit = audioUnit;

    checkStatus(AudioUnitSetProperty(outputAudioUnit,
                              kAudioUnitProperty_MakeConnection,
                              kAudioUnitScope_Input,
                              0,
                              &connection,
                              sizeof(connection)));

    
    return noErr;
}

OSStatus AudioPlayThrough::setupBuffers(){
    
    UInt32 bufferSizeFrames,bufferSizeBytes,propsize;
    
    AudioStreamBasicDescription asbd,asbd_dev1_in,asbd_dev2_out;
    
    UInt32 propertySize = sizeof(bufferSizeFrames);
    checkStatus(AudioUnitGetProperty(inputAudioUnit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &bufferSizeFrames, &propertySize));
    
    bufferSizeBytes = bufferSizeFrames * sizeof(Float32);
        
    propertySize = sizeof(asbd_dev1_in);
    checkStatus(AudioUnitGetProperty(inputAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 1, &asbd_dev1_in, &propertySize));
    

    propertySize = sizeof(asbd);
    checkStatus(AudioUnitGetProperty(outputAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 1, &asbd, &propertySize));
    

    propertySize = sizeof(asbd_dev2_out);
    checkStatus(AudioUnitGetProperty(outputAudioUnit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &asbd_dev2_out, &propertySize));
    

    asbd.mChannelsPerFrame =((asbd_dev1_in.mChannelsPerFrame < asbd_dev2_out.mChannelsPerFrame) ? asbd_dev1_in.mChannelsPerFrame :asbd_dev2_out.mChannelsPerFrame);

    //calculate number of buffers from channels
    propsize = offsetof(AudioBufferList, mBuffers[0]) + (sizeof(AudioBuffer) *inputAudioStreamBasicDescription.mChannelsPerFrame);

    //malloc buffer lists
    inputBuffer = (AudioBufferList *)malloc(propsize);
    inputBuffer->mNumberBuffers = inputAudioStreamBasicDescription.mChannelsPerFrame;
    
    //pre-malloc buffers for AudioBufferLists
    for(UInt32 i =0; i< inputBuffer->mNumberBuffers ; i++) {
        inputBuffer->mBuffers[i].mNumberChannels = 1;
        inputBuffer->mBuffers[i].mDataByteSize = bufferSizeBytes;
        inputBuffer->mBuffers[i].mData = malloc(bufferSizeBytes);
    }
    
    // alloc rings buffers.
    ringBuffer = (Float32*)calloc(ringBufferFrameSize * inputAudioStreamBasicDescription.mChannelsPerFrame, sizeof(Float32));
    
    return noErr;
}

OSStatus AudioPlayThrough::initializeAudioUnits(){
    
    // AU needs to be initialized before we start them
    checkStatus(AudioUnitInitialize(inputAudioUnit));
    checkStatus(AudioUnitInitialize(varispeedAudioUnit));
    checkStatus(AudioUnitInitialize(multiChannelMixerAudioUnit));
    checkStatus(AudioUnitInitialize(audioUnit));
    checkStatus(AudioUnitInitialize(outputAudioUnit));
    
    
    return noErr;
}
                
OSStatus AudioPlayThrough::addInputListener(){

    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyStreams,
                                              kAudioObjectPropertyScopeGlobal,
                                              kAudioObjectPropertyElementMaster };
    
    UInt32 size;
    checkStatus(AudioObjectGetPropertyDataSize(inputAudioDeviceID, &theAddress, 0, NULL, &size));
    
    UInt32 numStreams = size / sizeof(AudioStreamID);
    AudioStreamID streams[numStreams];
    checkStatus(AudioObjectGetPropertyData(inputAudioDeviceID, &theAddress, 0, NULL, &size, streams));
    
    for(UInt32 i=0; i < numStreams; i++) {
        UInt32 isInput;
        size = sizeof(UInt32);
        theAddress.mSelector = kAudioStreamPropertyDirection;
        theAddress.mScope = kAudioObjectPropertyScopeGlobal;
        
        checkStatus(AudioObjectGetPropertyData(streams[i], &theAddress, 0, NULL, &size, &isInput));

        if(isInput) {
            
            theAddress.mSelector = kAudioDevicePropertyStreamFormat;
            checkStatus(AudioObjectAddPropertyListener(streams[i], &theAddress, streamListenerProc, this));
        }
    }
    
    theAddress.mSelector = kAudioDevicePropertyBufferSize;
    theAddress.mScope = kAudioObjectPropertyScopeWildcard;
    theAddress.mElement = kAudioObjectPropertyElementWildcard;
    
    checkStatus(AudioObjectAddPropertyListener(inputAudioDeviceID, &theAddress, streamListenerProc, this));

    return noErr;
}

OSStatus AudioPlayThrough::removeInputListener(){
    
    if (inputAudioDeviceID == 0) {
        return 0;
    }

    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyStreams,
                                              kAudioObjectPropertyScopeGlobal,
                                              kAudioObjectPropertyElementMaster };
    
    UInt32 size;
    checkStatus(AudioObjectGetPropertyDataSize(inputAudioDeviceID, &theAddress, 0, NULL, &size));
    
    UInt32 numStreams = size / sizeof(AudioStreamID);
    AudioStreamID streams[numStreams];
    checkStatus(AudioObjectGetPropertyData(inputAudioDeviceID, &theAddress, 0, NULL, &size, streams));
    
    for(UInt32 i=0; i < numStreams; i++) {
        UInt32 isInput;
        size = sizeof(UInt32);
        theAddress.mSelector = kAudioStreamPropertyDirection;
        theAddress.mScope = kAudioObjectPropertyScopeGlobal;
        
        checkStatus(AudioObjectGetPropertyData(streams[i], &theAddress, 0, NULL, &size, &isInput));

        if(isInput) {
            
            theAddress.mSelector = kAudioDevicePropertyStreamFormat;
            checkStatus(AudioObjectRemovePropertyListener(streams[i], &theAddress, streamListenerProc, this));
        }
    }
    
    theAddress.mSelector = kAudioDevicePropertyBufferSize;
    theAddress.mScope = kAudioObjectPropertyScopeWildcard;
    theAddress.mElement = kAudioObjectPropertyElementWildcard;
    
    checkStatus(AudioObjectRemovePropertyListener(inputAudioDeviceID, &theAddress, streamListenerProc, this));

    return noErr;
}

OSStatus AudioPlayThrough::addOutputListener(){

    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyStreams,
                                              kAudioObjectPropertyScopeGlobal,
                                              kAudioObjectPropertyElementMaster };
    
    UInt32 size;
    checkStatus(AudioObjectGetPropertyDataSize(outputAudioDeviceID, &theAddress, 0, NULL, &size));
    
    UInt32 numStreams = size / sizeof(AudioStreamID);
    AudioStreamID streams[numStreams];
    checkStatus(AudioObjectGetPropertyData(outputAudioDeviceID, &theAddress, 0, NULL, &size, streams));
    
    for(UInt32 i=0; i < numStreams; i++) {
        UInt32 isInput;
        size = sizeof(UInt32);
        theAddress.mSelector = kAudioStreamPropertyDirection;
        theAddress.mScope = kAudioObjectPropertyScopeGlobal;
        
        checkStatus(AudioObjectGetPropertyData(streams[i], &theAddress, 0, NULL, &size, &isInput));

        if(!isInput) {
            
            theAddress.mSelector = kAudioDevicePropertyStreamFormat;
            checkStatus(AudioObjectAddPropertyListener(streams[i], &theAddress, streamListenerProc, this));
        }
    }
    
    theAddress.mSelector = kAudioDevicePropertyBufferSize;
    theAddress.mScope = kAudioObjectPropertyScopeWildcard;
    theAddress.mElement = kAudioObjectPropertyElementWildcard;
    
    // non fatal 
    AudioObjectAddPropertyListener(outputAudioDeviceID, &theAddress, streamListenerProc, this);

    return noErr;
}

OSStatus AudioPlayThrough::removeOutputListener(){
    
    if (outputAudioDeviceID == 0) {
        return 0;
    }

    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyStreams,
                                              kAudioObjectPropertyScopeGlobal,
                                              kAudioObjectPropertyElementMaster };
    
    UInt32 size;
    checkStatus(AudioObjectGetPropertyDataSize(outputAudioDeviceID, &theAddress, 0, NULL, &size));
    
    UInt32 numStreams = size / sizeof(AudioStreamID);
    AudioStreamID streams[numStreams];
    checkStatus(AudioObjectGetPropertyData(outputAudioDeviceID, &theAddress, 0, NULL, &size, streams));
    
    for(UInt32 i=0; i < numStreams; i++) {
        UInt32 isInput;
        size = sizeof(UInt32);
        theAddress.mSelector = kAudioStreamPropertyDirection;
        theAddress.mScope = kAudioObjectPropertyScopeGlobal;
        
        checkStatus(AudioObjectGetPropertyData(streams[i], &theAddress, 0, NULL, &size, &isInput));

        if(!isInput) {
            
            theAddress.mSelector = kAudioDevicePropertyStreamFormat;
            checkStatus(AudioObjectRemovePropertyListener(streams[i], &theAddress, streamListenerProc, this));
        }
    }
    
    theAddress.mSelector = kAudioDevicePropertyBufferSize;
    theAddress.mScope = kAudioObjectPropertyScopeWildcard;
    theAddress.mElement = kAudioObjectPropertyElementWildcard;
    
    checkStatus(AudioObjectRemovePropertyListener(outputAudioDeviceID, &theAddress, streamListenerProc, this));

    return noErr;
}

OSStatus AudioPlayThrough::addDeviceIsAliveListener(AudioDeviceID audioDeviceID){

    // Remove any listeners first.
    removeDeviceIsAliveListener(audioDeviceID);
    
    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyDeviceIsAlive,
                                              kAudioObjectPropertyScopeGlobal,
                                              kAudioObjectPropertyElementMaster };
    
    checkStatus(AudioObjectAddPropertyListener(audioDeviceID, &theAddress, deviceIsAliveListenerProc, this));

    return noErr;
}

OSStatus AudioPlayThrough::removeDeviceIsAliveListener(AudioDeviceID audioDeviceID){
    
    if (audioDeviceID == 0) {
        return 0;
    }

    AudioObjectPropertyAddress theAddress = { kAudioDevicePropertyDeviceIsAlive,
                                              kAudioObjectPropertyScopeGlobal,
                                              kAudioObjectPropertyElementMaster };
    
    checkStatus(AudioObjectRemovePropertyListener(audioDeviceID, &theAddress, deviceIsAliveListenerProc, this));

    return noErr;
}

OSStatus AudioPlayThrough::deviceIsAliveListenerProc(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress *inAddresses, void *inClientData){
    
    AudioPlayThrough* This = (AudioPlayThrough*)inClientData;
    
    This->takedown();
    
    return noErr;
}


OSStatus AudioPlayThrough::takedown(){
    
    // If we free memory before the callbacks stop it will crash.
    // The callbacks get called a few times before they actually stop.
    
    dispatch_sync(queue, ^{
        
        // free memory
        if (inputBuffer != NULL) {
            for (UInt32 buffer = 0; buffer < inputBuffer->mNumberBuffers; buffer++)
            {
                if (inputBuffer->mBuffers[buffer].mData != NULL){
                    free(inputBuffer->mBuffers[buffer].mData);
                    inputBuffer->mBuffers[buffer].mData = NULL;
                }
            }
            free(inputBuffer);
            inputBuffer = NULL;
        }
        
        if (ringBuffer != NULL) {
            free(ringBuffer);
            ringBuffer = NULL;
        }
        
        // remove listeners
        removeInputListener();
        removeOutputListener();
        removeDeviceIsAliveListener(inputAudioDeviceID);
        removeDeviceIsAliveListener(outputAudioDeviceID);
        
        inputAudioDeviceID = 0;
        outputAudioDeviceID = 0;
        
        AudioUnitUninitialize(inputAudioUnit);
        AudioUnitUninitialize(outputAudioUnit);
        AudioUnitUninitialize(varispeedAudioUnit);
        AudioUnitUninitialize(audioUnit);
        
    });
    
    return noErr;
}
