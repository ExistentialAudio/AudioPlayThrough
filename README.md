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
If you want to use this with Swift I recommend using C++ Interoperability. 
