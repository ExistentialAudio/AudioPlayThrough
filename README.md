# AudioPlayThrough

## C++ API

```
#include "AudioPlayThrough.h

// Setup
OSStatus status;
AudioPlayThrough audioPlayThrough = AudioPlayThrough();
status = audioPlayThrough.create(inputUID, outputUID);

// Add an AudioUnit for signal processing
audioPlayThrough.setAudioUnit(AudioUnit audioUnit);
audioPlayThrough.bypassAudioUnit(UInt32 value);

// Basic Controls
status = audioPlayThrough.start();
status = audioPlayThrough.stop();

// If set, this will be called every buffer with the highest peak. 
audioPlayThrough.peakCallback = (Float32 peak){};

// Set initial levels. To pass audio from channel 1 out to channel 1 at 1.0 use the following.
status = audioPlayThrough.setMatrixLevel(0, 0, 1.0);

```

## C Wrapper for Swift

Unfortunately using ObjC to wrap audio code results in numerous audio errors. The safest method to ensure high quality real-time audio is to use a C wrapper. 

```
import AudioPlayThroughC

var audioPlayThrough: UnsafeMutableRawPointer?

audioPlayThrough = AudioPlayThroughCreate("inputUID" as CFString, "outputUID" as CFString)

let status = AudioPlayThroughStart(audioPlayThrough)

// Set initial volume and routing.
AudioPlayThroughSetMatrixLevel(
    audioPlayThrough,
    UInt32(0),     // input channel
    UInt32(0),     // output channel
    Float32(1.0)   // volume
)
AudioPlayThroughSetMatrixLevel(
    audioPlayThrough,
    UInt32(1),     // input channel
    UInt32(1),     // output channel
    Float32(1.0)   // volume
)

status = AudioPlayThroughStop(audioPlayThrough)

```

