#ifdef __cplusplus
extern "C"  {
#endif

#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>

void AudioPlayThroughCreate(CFStringRef inputUID, CFStringRef outputUID);
void AudioPlayThroughSetAudioUnit(AudioUnit audioUnit);
OSStatus AudioPlayThroughStart(void);
OSStatus AudioPlayThroughStop(void);

#ifdef __cplusplus
}
#endif
