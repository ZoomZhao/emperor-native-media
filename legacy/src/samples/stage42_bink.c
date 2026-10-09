/* Authored failure-path Bink platform adapter. No original DLL code executed.
 * Real own-module lookup and readonly POSIX IO; no decoder or success handles.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
extern int s37_stats(void*,uint32_t*);
enum {OK=0,INVALID=1,CLOSED=2,UNSUPPORTED=5,IOERROR=6};
typedef struct {uint32_t magic,version,guest_factory,driver,factory_calls,minimum_entry_found,sound_open_registered,open_calls,io_attempts,last_errno;char error[256],last_path[260];} Image;
typedef struct State State;
struct State {void*audio;void*sample_module;Image im;int(*factory)(State*,uint32_t,uint32_t*);};
static int live(State*s){uint32_t a[16];return s&&s37_stats(s->audio,a)==0&&a[5]&&a[1]==0&&(a[0]==1||a[0]==3);}
static int miles_factory(State*s,uint32_t driver,uint32_t*out){s->im.factory_calls++;*out=0;if(!driver)return OK;if(driver!=0x5c000101u||!live(s))return CLOSED;s->im.driver=driver;
 /* The fixed original Bink factory explicitly tests this first resolved entry.
  * Our reviewed native pool exports no such implementation; check the module
  * itself instead of manufacturing a sound-open callback table. */
 void*minimum=dlsym(s->sample_module,"s39_minimum_sample_buffer_size");s->im.minimum_entry_found=minimum!=NULL;
 if(!minimum)return OK;
 return UNSUPPORTED; /* Full real PCM callback ABI must be implemented first. */
}
void*s42_new(void*audio,const char*sample_library){if(!audio||!sample_library)return NULL;State*s=calloc(1,sizeof(*s));if(!s)return NULL;s->sample_module=dlopen(sample_library,RTLD_NOW|RTLD_LOCAL);if(!s->sample_module){free(s);return NULL;}if(!dlsym(s->sample_module,"s39_allocate")||!dlsym(s->sample_module,"s39_query")){dlclose(s->sample_module);free(s);return NULL;}s->audio=audio;s->im.magic=0x53343242;s->im.version=1;s->factory=miles_factory;return s;}
int s42_set_sound(void*p,uint32_t guest_factory,uint32_t driver,uint32_t*out){State*s=p;if(!s||!out||guest_factory!=0x61001a70u)return INVALID;if(s->im.guest_factory&&s->im.guest_factory!=guest_factory&&s->im.sound_open_registered){*out=0;return OK;}s->im.guest_factory=guest_factory;uint32_t result=0;int status=s->factory(s,driver,&result);if(status)return status;if(result)return UNSUPPORTED;s->im.sound_open_registered=0;*out=s->im.sound_open_registered;return OK;}
int s42_open_missing(void*p,const char*root,const char*relative,uint32_t flags,uint32_t*out){State*s=p;if(!s||!root||!relative||!out||flags!=0x08000000u)return INVALID;if(strcmp(relative,"Binks/emperor_intro.bik")&&strcmp(relative,"Binks/emperor_intro_hiq.bik"))return INVALID;char path[4096];if(snprintf(path,sizeof(path),"%s/%s",root,relative)>=(int)sizeof(path))return INVALID;s->im.error[0]=0;s->im.open_calls++;strcpy(s->im.last_path,relative);s->im.io_attempts=0;s->im.last_errno=0;
 for(int i=0;i<2;i++){s->im.io_attempts++;int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd>=0){close(fd);return UNSUPPORTED;}s->im.last_errno=(uint32_t)errno;if(errno!=ENOENT)return IOERROR;}
 strcpy(s->im.error,"Error opening file.");*out=0;return OK;
}
int s42_error(void*p,char*out,uint32_t n){State*s=p;if(!s||!out||n<sizeof(s->im.error))return INVALID;memcpy(out,s->im.error,sizeof(s->im.error));return OK;}
int s42_capabilities(void*p,uint32_t*out){State*s=p;if(!s||!out)return INVALID;out[0]=live(s);out[1]=dlsym(s->sample_module,"s39_allocate")!=NULL;out[2]=dlsym(s->sample_module,"s39_minimum_sample_buffer_size")!=NULL;out[3]=s->im.factory_calls;out[4]=s->im.sound_open_registered;out[5]=s->im.open_calls;out[6]=s->im.io_attempts;out[7]=s->im.last_errno;return OK;}
size_t s42_size(void){return sizeof(Image);}
int s42_export(void*p,void*out,size_t n){State*s=p;if(!s||!out||n!=sizeof(Image))return INVALID;memcpy(out,&s->im,n);return OK;}
int s42_import(void*p,const void*in,size_t n){State*s=p;if(!s||!in||n!=sizeof(Image))return INVALID;const Image*i=in;if(i->magic!=0x53343242||i->version!=1||(i->guest_factory&&i->guest_factory!=0x61001a70u)||(i->driver&&i->driver!=0x5c000101u)||i->minimum_entry_found||i->sound_open_registered||i->io_attempts>2||!memchr(i->error,0,sizeof(i->error))||!memchr(i->last_path,0,sizeof(i->last_path)))return INVALID;if(dlsym(s->sample_module,"s39_minimum_sample_buffer_size"))return UNSUPPORTED;memcpy(&s->im,i,n);s->factory=miles_factory;return OK;}
void s42_delete(void*p){State*s=p;if(s){if(s->sample_module)dlclose(s->sample_module);free(s);}}
