/* Authored capability-backed registry. No original provider code or names. */
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
extern int s37_stats(void*,uint32_t*);
enum {OK=0,INVALID=1,CLOSED=2,UNSUPPORTED=5};
enum {PCM=1,MSS3D=2};
typedef struct {uint32_t magic,version,registered,generation,device,service_mask;char name[96],service[96];} Image;
typedef struct {void*audio;Image im;const char*(*attribute)(const Image*,int32_t);} State;
static const char*name_attribute(const Image*i,int32_t token){return token==-100?i->name:NULL;}
static uint32_t handle(const Image*i){return 0x60000001u|(i->generation<<8);}
static int live(State*s,uint32_t*out){uint32_t a[16];if(!s||s37_stats(s->audio,a)||!a[5]||a[1]||(a[0]!=1&&a[0]!=3))return 0;if(out)*out=a[5];return 1;}
void*s40_new(void*audio){if(!audio)return NULL;State*s=calloc(1,sizeof(*s));if(!s)return NULL;s->audio=audio;s->im.magic=0x53343052;s->im.version=1;s->im.generation=1;s->attribute=name_attribute;return s;}
int s40_start(void*p){State*s=p;uint32_t device;if(!s)return INVALID;if(s->im.registered)return OK;if(!live(s,&device))return CLOSED;s->im.device=device;s->im.service_mask=PCM;strcpy(s->im.name,"Authored macOS CoreAudio PCM queue");strcpy(s->im.service,"Codex CoreAudio PCM queue");s->im.registered=1;return OK;}
int s40_shutdown(void*p){State*s=p;if(!s||!s->im.registered||s->im.generation==65535)return INVALID;s->im.registered=0;s->im.device=s->im.service_mask=0;s->im.generation++;memset(s->im.name,0,sizeof(s->im.name));memset(s->im.service,0,sizeof(s->im.service));return OK;}
int s40_enumerate(void*p,const char*service,uint32_t*cursor,uint32_t*out){State*s=p;if(!s||!cursor||!out)return -INVALID;uint32_t candidate=*cursor==0?1:*cursor+1;if(s->im.registered&&candidate==1&&(!service||strcmp(service,s->im.service)==0)){if(!live(s,NULL))return -CLOSED;*cursor=1;*out=handle(&s->im);return 1;}*cursor=0;*out=0;return 0;}
int s40_name(void*p,uint32_t h,int32_t token,char*out,uint32_t n){State*s=p;if(!s||!out||!n||!s->im.registered||h!=handle(&s->im)||!live(s,NULL))return INVALID;const char*t=s->attribute(&s->im,token);if(!t)return UNSUPPORTED;size_t z=strlen(t)+1;if(n<z)return INVALID;memcpy(out,t,z);return OK;}
int s40_register_mss3d(void*p){State*s=p;if(!s)return INVALID;/* No listener/sample/position callback table is implemented in this backend. */return UNSUPPORTED;}
int s40_capabilities(void*p,uint32_t*out,char*spatial_name,uint32_t n){State*s=p;if(!s||!out||!spatial_name||!n)return INVALID;memset(out,0,12*4);out[0]=s->im.registered;out[1]=s->im.device;out[2]=s->im.service_mask;out[3]=(s->im.service_mask&MSS3D)!=0;out[4]=live(s,NULL);out[5]=s->im.registered?handle(&s->im):0;spatial_name[0]=0;AudioComponentDescription d={kAudioUnitType_Mixer,kAudioUnitSubType_SpatialMixer,kAudioUnitManufacturer_Apple,0,0};AudioComponent c=NULL;while((c=AudioComponentFindNext(c,&d))){out[6]++;if(out[6]==1){CFStringRef name=NULL;if(AudioComponentCopyName(c,&name)==noErr&&name){CFStringGetCString(name,spatial_name,n,kCFStringEncodingUTF8);CFRelease(name);}UInt32 v=0;if(AudioComponentGetVersion(c,&v)==noErr)out[7]=v;}}return OK;}
size_t s40_size(void){return sizeof(Image);}
int s40_export(void*p,void*out,size_t n){State*s=p;if(!s||!out||n!=sizeof(Image))return INVALID;memcpy(out,&s->im,n);return OK;}
int s40_import(void*p,const void*in,size_t n){State*s=p;if(!s||!in||n!=sizeof(Image))return INVALID;const Image*i=in;uint32_t device=0;if(i->magic!=0x53343052||i->version!=1||i->registered>1||!i->generation||i->generation>65535)return INVALID;if(i->registered){if(!live(s,&device)||i->device!=device||i->service_mask!=PCM||strcmp(i->name,"Authored macOS CoreAudio PCM queue")||strcmp(i->service,"Codex CoreAudio PCM queue"))return INVALID;}else if(i->device||i->service_mask||memcmp(i->name,(char[96]){0},96)||memcmp(i->service,(char[96]){0},96))return INVALID;memcpy(&s->im,i,n);s->attribute=name_attribute;return OK;}
void s40_delete(void*p){free(p);}
