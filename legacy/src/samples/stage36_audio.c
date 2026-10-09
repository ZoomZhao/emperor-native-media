/* Authored host-output plumbing. No original Miles or provider code. */
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudio.h>
#include <mach/mach_time.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef struct { AudioUnit unit; AudioDeviceID device; AudioStreamBasicDescription pcm; _Atomic uint64_t callbacks,frames,bytes,first_host,last_host; _Atomic uint32_t bad_buffers; uint32_t initialized,started,generation; OSStatus last; } S;
typedef struct {uint32_t magic,version,rate,channels,bits,device,initialized,started,generation,bad_buffers;uint64_t callbacks,frames,bytes,first_host,last_host;} Image;
static OSStatus render(void*ref,AudioUnitRenderActionFlags*flags,const AudioTimeStamp*time,UInt32 bus,UInt32 frames,AudioBufferList*data){
 S*s=ref;if(!data||data->mNumberBuffers!=1||!data->mBuffers[0].mData||data->mBuffers[0].mNumberChannels!=2||data->mBuffers[0].mDataByteSize!=frames*4){atomic_fetch_add(&s->bad_buffers,1);return kAudio_ParamError;}
 memset(data->mBuffers[0].mData,0,data->mBuffers[0].mDataByteSize);*flags|=kAudioUnitRenderAction_OutputIsSilence;
 uint64_t host=time->mFlags&kAudioTimeStampHostTimeValid?time->mHostTime:mach_absolute_time();uint64_t empty=0;atomic_compare_exchange_strong(&s->first_host,&empty,host);atomic_store(&s->last_host,host);atomic_fetch_add(&s->callbacks,1);atomic_fetch_add(&s->frames,frames);atomic_fetch_add(&s->bytes,frames*4);return noErr;
}
void*s36_new(void){return calloc(1,sizeof(S));}
int s36_open(void*p){
 S*s=p;if(!s||s->unit)return 0;AudioObjectPropertyAddress a={kAudioHardwarePropertyDefaultOutputDevice,kAudioObjectPropertyScopeGlobal,kAudioObjectPropertyElementMain};UInt32 n=sizeof(s->device);s->last=AudioObjectGetPropertyData(kAudioObjectSystemObject,&a,0,NULL,&n,&s->device);if(s->last||!s->device)return 0;
 AudioComponentDescription d={kAudioUnitType_Output,kAudioUnitSubType_DefaultOutput,kAudioUnitManufacturer_Apple,0,0};AudioComponent c=AudioComponentFindNext(NULL,&d);if(!c)return 0;s->last=AudioComponentInstanceNew(c,&s->unit);if(s->last)return 0;
 s->pcm=(AudioStreamBasicDescription){22050,kAudioFormatLinearPCM,kLinearPCMFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked,4,1,4,2,16,0};s->last=AudioUnitSetProperty(s->unit,kAudioUnitProperty_StreamFormat,kAudioUnitScope_Input,0,&s->pcm,sizeof(s->pcm));if(s->last)return 0;
 AURenderCallbackStruct cb={render,s};s->last=AudioUnitSetProperty(s->unit,kAudioUnitProperty_SetRenderCallback,kAudioUnitScope_Input,0,&cb,sizeof(cb));if(s->last)return 0;s->last=AudioUnitInitialize(s->unit);if(s->last)return 0;s->initialized=1;s->generation++;return 1;
}
int s36_start(void*p){S*s=p;if(!s||!s->initialized||s->started)return 0;s->last=AudioOutputUnitStart(s->unit);if(s->last)return 0;s->started=1;return 1;}
int s36_stop(void*p){S*s=p;if(!s||!s->started)return 0;s->last=AudioOutputUnitStop(s->unit);if(s->last)return 0;s->started=0;return 1;}
int s36_status(void*p,uint32_t*out){S*s=p;if(!s||!out)return 0;out[0]=(uint32_t)s->last;out[1]=s->device;out[2]=s->initialized;out[3]=s->started;out[4]=s->generation;return 1;}
size_t s36_size(void){return sizeof(Image);}
int s36_export(void*p,void*out,size_t n){S*s=p;if(!s||!out||n!=sizeof(Image)||s->started)return 0;Image im={0};im.magic=0x53333641;im.version=1;im.rate=22050;im.channels=2;im.bits=16;im.device=s->device;im.initialized=s->initialized;im.started=s->started;im.generation=s->generation;im.bad_buffers=atomic_load(&s->bad_buffers);im.callbacks=atomic_load(&s->callbacks);im.frames=atomic_load(&s->frames);im.bytes=atomic_load(&s->bytes);im.first_host=atomic_load(&s->first_host);im.last_host=atomic_load(&s->last_host);memcpy(out,&im,n);return 1;}
int s36_import(void*p,const void*in,size_t n){S*s=p;if(!s||!in||n!=sizeof(Image)||s->unit)return 0;Image im;memcpy(&im,in,n);if(im.magic!=0x53333641||im.version!=1||im.rate!=22050||im.channels!=2||im.bits!=16||!im.initialized||im.started||im.bad_buffers||im.bytes!=im.frames*4)return 0;if(!s36_open(s)||s->device!=im.device)return 0;s->generation=im.generation;atomic_store(&s->callbacks,im.callbacks);atomic_store(&s->frames,im.frames);atomic_store(&s->bytes,im.bytes);atomic_store(&s->first_host,im.first_host);atomic_store(&s->last_host,im.last_host);return 1;}
void s36_delete(void*p){S*s=p;if(!s)return;if(s->unit){if(s->started)AudioOutputUnitStop(s->unit);if(s->initialized)AudioUnitUninitialize(s->unit);AudioComponentInstanceDispose(s->unit);}free(s);}
