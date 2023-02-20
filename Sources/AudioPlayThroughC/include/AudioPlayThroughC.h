#ifdef __cplusplus
extern "C"  {
#endif

#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>

void* AudioPlayThroughCreate(CFStringRef inputUID, CFStringRef outputUID);
OSStatus AudioPlayThroughStart(void* audioPlayThrough);
OSStatus AudioPlayThroughStop(void* audioPlayThrough);
void AudioPlayThroughSetPeakCallback(void* audioPlayThrough, void(*peakCallback)(Float32 peak));
Float32 AudioPlayThroughGetPeak(void* audioPlayThrough);
OSStatus AudioPlayThroughSetMatrixLevel(void* audioPlayThrough, UInt32 inputChannel, UInt32 outputChannel, Float32 level);

const char* GetBuildDate();

#ifdef __cplusplus
}
#endif
