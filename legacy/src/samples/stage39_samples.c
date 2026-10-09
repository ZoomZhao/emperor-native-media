/* Authored empty sample pool bound to a live Stage37 CoreAudio driver.
 * Only empty-object lifecycle is supported; nonempty/mixer/codec calls refuse.
 * No original instructions, native pointers or original providers in snapshots.
 */
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
extern int s37_stats(void*,uint32_t*);
#define CAP 64
#define DRIVER 0x5c000101u
#define MIXER 0x5f000001u
enum {OK=0,INVALID=1,FULL=3,UNSUPPORTED=5};
typedef struct {uint32_t generation,words[161];} Slot;
typedef struct {uint32_t magic,version,driver,capacity,volume,master,format,pan[128];uint64_t calls;Slot slots[CAP];} Image;
typedef struct {pthread_mutex_t lock;void*audio;Image image;} State;
static uint32_t handle(State*s,int i){return 0x5e000000u|(s->image.slots[i].generation<<8)|(i+1);}
static int lookup(State*s,uint32_t h){int i=(h&255)-1;return i>=0&&i<(int)s->image.capacity&&h==handle(s,i)&&s->image.slots[i].words[2]!=1?i:-1;}
static int backend(State*s){uint32_t out[16];return s->audio&&s37_stats(s->audio,out)==0&&out[5]!=0&&(out[0]==1||out[0]==3)&&!out[1];}
static int clamp(int v){return v<0?0:v>127?127:v;}
static void gains(State*s,uint32_t*w){int v=clamp((int)w[23]);int p=clamp((int)w[24]);w[23]=v;w[24]=p;int effective=clamp((int)s->image.master*v/127);int left=s->image.pan[127-p]*effective/8,right=s->image.pan[p]*effective/8,mono=effective*16;w[25]=left>=2032?2047:left;w[26]=right>=2032?2047:right;w[160]=mono>=2032?2047:mono;}
static int empty(const uint32_t*w){return !w[3]&&!w[4]&&!w[5]&&!w[6]&&!w[28]&&!w[29]&&!w[30]&&!w[79]&&!w[105];}
static void initialize(State*s,int i){uint32_t*w=s->image.slots[i].words;w[2]=1;for(int j=3;j<=9;j++)w[j]=0;w[10]=1;for(int j=11;j<=14;j++)w[j]=0;w[15]=0xfffffffeu;w[16]=w[17]=1;w[18]=0;w[19]=0xffffffffu;w[20]=w[21]=0;w[22]=11025;w[23]=s->image.volume;w[47]=256;w[48]=w[49]=0;w[24]=s->image.format>1?64:0;w[28]=w[29]=w[30]=0;w[76]=w[77]=w[78]=0;w[157]=0;w[158]=0x3cf5c28f;w[159]=0x3fbf1aa0;gains(s,w);w[131]=1;w[132]=MIXER;w[134]=0x5f000002u;w[105]=w[106]=0;w[79]=w[80]=0;w[2]=2;}
void*s39_new(void*audio,uint32_t cap,uint32_t volume,const uint32_t*pan){if(!audio||!pan||cap!=CAP||volume>127)return NULL;State*s=calloc(1,sizeof(*s));if(!s)return NULL;if(pthread_mutex_init(&s->lock,NULL)){free(s);return NULL;}s->audio=audio;s->image.magic=0x53333953;s->image.version=1;s->image.capacity=cap;s->image.volume=volume;s->image.master=127;s->image.format=3;memcpy(s->image.pan,pan,128*4);for(int i=0;i<CAP;i++){s->image.slots[i].generation=1;s->image.slots[i].words[0]=0x5333394f;s->image.slots[i].words[1]=DRIVER;s->image.slots[i].words[2]=1;}return s;}
void s39_delete(void*p){State*s=p;if(s){pthread_mutex_destroy(&s->lock);free(s);}}
int s39_allocate(void*p,uint32_t driver,uint32_t*out){State*s=p;if(!s||!out||driver!=DRIVER||!backend(s))return INVALID;pthread_mutex_lock(&s->lock);int status=FULL;s->image.driver=DRIVER;for(int i=0;i<CAP;i++)if(s->image.slots[i].words[2]==1){initialize(s,i);*out=handle(s,i);status=OK;break;}s->image.calls++;pthread_mutex_unlock(&s->lock);return status;}
int s39_init(void*p,uint32_t h){State*s=p;if(!s||!backend(s))return INVALID;pthread_mutex_lock(&s->lock);int i=lookup(s,h),status=INVALID;if(i>=0){if(empty(s->image.slots[i].words)){initialize(s,i);status=OK;s->image.calls++;}else status=UNSUPPORTED;}pthread_mutex_unlock(&s->lock);return status;}
int s39_release(void*p,uint32_t h){State*s=p;if(!s)return INVALID;pthread_mutex_lock(&s->lock);int i=lookup(s,h),status=INVALID;if(i>=0){Slot*b=&s->image.slots[i];if(!empty(b->words))status=UNSUPPORTED;else if(b->generation<65535){b->words[2]=1;b->words[79]=b->words[105]=b->words[131]=0;b->words[80]=b->words[106]=b->words[132]=0;b->generation++;s->image.calls++;status=OK;}}pthread_mutex_unlock(&s->lock);return status;}
int s39_query(void*p,uint32_t h,uint32_t field,uint32_t*out){State*s=p;if(!s||!out||field>=161)return INVALID;pthread_mutex_lock(&s->lock);int i=lookup(s,h),status=INVALID;if(i>=0){*out=s->image.slots[i].words[field];status=OK;}pthread_mutex_unlock(&s->lock);return status;}
int s39_set(void*p,uint32_t h,uint32_t field,uint32_t value){State*s=p;if(!s||(field!=17&&field!=22&&field!=23&&field!=24))return INVALID;pthread_mutex_lock(&s->lock);int i=lookup(s,h),status=INVALID;if(i>=0){uint32_t*w=s->image.slots[i].words;if(!empty(w))status=UNSUPPORTED;else{w[field]=value;if(field==23||field==24)gains(s,w);s->image.calls++;status=OK;}}pthread_mutex_unlock(&s->lock);return status;}
int s39_start_empty(void*p,uint32_t h){State*s=p;if(!s||!backend(s))return INVALID;pthread_mutex_lock(&s->lock);int i=lookup(s,h),status=INVALID;if(i>=0){uint32_t*w=s->image.slots[i].words;if(empty(w)&&w[2]==2){status=OK;s->image.calls++;}else status=UNSUPPORTED;}pthread_mutex_unlock(&s->lock);return status;}
size_t s39_size(void){return sizeof(Image);}
int s39_export(void*p,void*out,size_t n){State*s=p;if(!s||!out||n!=sizeof(Image))return INVALID;pthread_mutex_lock(&s->lock);memcpy(out,&s->image,n);pthread_mutex_unlock(&s->lock);return OK;}
int s39_import(void*p,const void*in,size_t n){State*s=p;if(!s||!in||n!=sizeof(Image))return INVALID;const Image*im=in;if(im->magic!=0x53333953||im->version!=1||im->capacity!=CAP||im->volume>127||im->master!=127||im->format!=3||(im->driver&&im->driver!=DRIVER)||memcmp(im->pan,s->image.pan,128*4)||(im->driver&&!backend(s)))return INVALID;for(int i=0;i<CAP;i++){const Slot*b=&im->slots[i];if(!b->generation||b->generation>65535||b->words[0]!=0x5333394f||b->words[1]!=DRIVER||(b->words[2]!=1&&b->words[2]!=2)||!empty(b->words))return INVALID;if(b->words[2]==2&&(!b->words[131]||b->words[132]!=MIXER||b->words[134]!=0x5f000002u))return INVALID;}pthread_mutex_lock(&s->lock);memcpy(&s->image,in,n);pthread_mutex_unlock(&s->lock);return OK;}
