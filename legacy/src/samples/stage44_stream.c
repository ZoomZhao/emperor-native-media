/* Self-authored offline PCM stream. Never feeds source PCM to an audio device. */
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#define QCAP 64000
#define MAXFRAMES (22050u*240u)
extern int s37_stats(void*,uint32_t*);
typedef struct {uint32_t magic,version,generation,handle,status,loops,volume,capacity,head,queued,source,passes,frames;uint64_t decoded_hash,consumed,mixed_hash,last_clock,nonzero;char path[1024];int16_t queue[QCAP*2];} Image;
typedef struct {Image im;ExtAudioFileRef file;int16_t*pcm;void*audio;} State;
static uint64_t hash(uint64_t h,const unsigned char*p,size_t n){for(size_t i=0;i<n;i++){h^=p[i];h*=1099511628211ULL;}return h;}
static int decode(State*s,const char*path){CFURLRef url=CFURLCreateFromFileSystemRepresentation(NULL,(const UInt8*)path,strlen(path),false);if(!url)return -1;OSStatus e=ExtAudioFileOpenURL(url,&s->file);CFRelease(url);if(e)return (int)e;
 AudioStreamBasicDescription fmt={0};fmt.mSampleRate=22050;fmt.mFormatID=kAudioFormatLinearPCM;fmt.mFormatFlags=kAudioFormatFlagIsSignedInteger|kAudioFormatFlagIsPacked;fmt.mBytesPerPacket=4;fmt.mFramesPerPacket=1;fmt.mBytesPerFrame=4;fmt.mChannelsPerFrame=2;fmt.mBitsPerChannel=16;
 e=ExtAudioFileSetProperty(s->file,kExtAudioFileProperty_ClientDataFormat,sizeof(fmt),&fmt);if(e)return (int)e;s->pcm=calloc(MAXFRAMES,4);if(!s->pcm)return -2;uint32_t total=0;
 for(;;){UInt32 frames=8192;if(total+frames>MAXFRAMES)return -3;AudioBufferList b={1,{{2,frames*4,s->pcm+total*2}}};e=ExtAudioFileRead(s->file,&frames,&b);if(e)return (int)e;if(!frames)break;total+=frames;}
 if(!total)return -4;s->im.frames=total;s->im.decoded_hash=hash(14695981039346656037ULL,(unsigned char*)s->pcm,total*4);strcpy(s->im.path,path);return 0;}
static void release(State*s){if(s->file)ExtAudioFileDispose(s->file);s->file=NULL;free(s->pcm);s->pcm=NULL;}
static uint64_t clock_frames(State*s){uint32_t a[16];if(!s->audio||s37_stats(s->audio,a))return s->im.last_clock;return (uint64_t)a[8]|((uint64_t)a[9]<<32);}
void*s44_new(void*audio){State*s=calloc(1,sizeof(*s));if(s){s->audio=audio;s->im.magic=0x5334344d;s->im.version=1;s->im.generation=1;}return s;}
static void fill(State*s){Image*i=&s->im;while(i->queued<i->capacity){if(i->source==i->frames){if(i->loops&&i->passes>=i->loops)break;i->passes++;if(i->loops&&i->passes>=i->loops)break;i->source=0;}uint32_t tail=(i->head+i->queued)%i->capacity;s->im.queue[tail*2]=s->pcm[i->source*2];s->im.queue[tail*2+1]=s->pcm[i->source*2+1];i->source++;i->queued++;}}
int s44_pull(State*s,uint32_t frames){if(!s||!s->im.handle)return -1;Image*i=&s->im;if(i->status!=4)return 0;for(uint32_t n=0;n<frames;n++){if(!i->queued){fill(s);if(!i->queued){i->status=2;break;}}int16_t pair[2];for(int ch=0;ch<2;ch++){pair[ch]=(int16_t)((int32_t)i->queue[i->head*2+ch]*(int32_t)i->volume/127);if(pair[ch])i->nonzero++;}i->mixed_hash=hash(i->mixed_hash,(unsigned char*)pair,4);i->head=(i->head+1)%i->capacity;i->queued--;i->consumed++;if(i->queued<i->capacity/2)fill(s);}if(!i->queued&&i->loops&&i->passes>=i->loops)i->status=2;return 0;}
int s44_service(State*s){if(!s)return -1;uint64_t c=clock_frames(s);if(c<s->im.last_clock)return -2;uint64_t delta=c-s->im.last_clock;s->im.last_clock=c;if(!s->im.handle)return 0;if(delta>22050*60)return -3;return s44_pull(s,(uint32_t)delta);}
int s44_open(State*s,const char*path,uint32_t bytes,uint32_t*out){if(!s||!path||!out||s->im.handle||strlen(path)>=1024||bytes<4||bytes>QCAP*4||bytes%4)return -1;int e=decode(s,path);if(e){release(s);return e;}s->im.handle=0x5f000000u|(s->im.generation<<8)|1;s->im.status=2;s->im.volume=127;s->im.loops=1;s->im.capacity=bytes/4;s->im.last_clock=clock_frames(s);s->im.mixed_hash=14695981039346656037ULL;*out=s->im.handle;return 0;}
int s44_command(State*s,uint32_t h,uint32_t command,uint32_t value){if(!s||!h||h!=s->im.handle)return -1;if(s44_service(s))return -2;Image*i=&s->im;
 if(command==1){i->loops=value;return 0;}if(command==2){if(value>127)return -3;i->volume=value;return 0;}if(command==3){i->head=i->queued=i->source=i->passes=0;i->consumed=i->nonzero=0;i->mixed_hash=14695981039346656037ULL;i->status=4;fill(s);return 0;}if(command==4){if(value>1)return -4;if(value&&i->status==4)i->status=8;else if(!value&&i->status==8)i->status=4;return 0;}if(command==5){release(s);uint32_t generation=i->generation+1;memset(i,0,sizeof(*i));i->magic=0x5334344d;i->version=1;i->generation=generation;i->last_clock=clock_frames(s);return 0;}return -5;}
int s44_status(State*s,uint32_t h){if(!s||h!=s->im.handle||!h)return -1;if(s44_service(s))return -2;return s->im.status;}
size_t s44_size(void){return sizeof(Image);}
int s44_export(State*s,void*out,size_t n){if(!s||n!=sizeof(Image)||s44_service(s))return -1;memcpy(out,&s->im,n);return 0;}
int s44_import(State*s,const void*p,size_t n){if(!s||n!=sizeof(Image))return -1;const Image*i=p;if(i->magic!=0x5334344d||i->version!=1||i->capacity>QCAP||i->queued>i->capacity||i->frames>MAXFRAMES||i->source>i->frames||!memchr(i->path,0,sizeof(i->path)))return -2;release(s);if(i->handle){int e=decode(s,i->path);if(e)return e;if(s->im.frames!=i->frames||s->im.decoded_hash!=i->decoded_hash)return -3;}s->im=*i;return 0;}
int s44_stats(State*s,uint64_t*out){if(!s||!out||s44_service(s))return -1;Image*i=&s->im;uint64_t v[]={i->handle,i->status,i->loops,i->volume,i->frames,i->decoded_hash,i->consumed,i->mixed_hash,i->queued,i->source,i->passes,i->last_clock,i->nonzero,s->file!=NULL};memcpy(out,v,sizeof(v));return 0;}
void s44_delete(State*s){if(s){release(s);free(s);}}
