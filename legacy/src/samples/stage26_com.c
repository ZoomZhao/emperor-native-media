/* Authored native guest COM apartment lifecycle, not a host Windows COM loader. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define INVALID UINT32_C(0x80070057)
#define MEMORY UINT32_C(0x8007000e)
#define CHANGED UINT32_C(0x80010106)
typedef struct {uint32_t tid,mode,count,reserved;} Apartment;
typedef struct {uint32_t magic,version;Apartment threads[32];} Image;
typedef struct {pthread_mutex_t mutex;Image image;} Core;
void *s26_new(void){Core*c=calloc(1,sizeof(*c));if(!c)return NULL;if(pthread_mutex_init(&c->mutex,NULL)){free(c);return NULL;}c->image.magic=0x434f4d32;c->image.version=1;return c;}
void s26_delete(Core*c){if(c){pthread_mutex_destroy(&c->mutex);free(c);}}
uint32_t s26_initialize(Core*c,uint32_t tid,uint32_t flags){
 if(!c||!tid||(flags&~14u))return INVALID;
 uint32_t mode=flags&2u,result=MEMORY;int found=0;pthread_mutex_lock(&c->mutex);Apartment*free_slot=NULL;
 for(unsigned i=0;i<32;i++){Apartment*a=&c->image.threads[i];if(a->tid==tid){found=1;if(a->mode!=mode){result=CHANGED;break;}if(a->count==UINT32_MAX){result=MEMORY;break;}a->count++;result=1;break;}if(!a->tid&&!free_slot)free_slot=a;}
 if(!found&&free_slot){*free_slot=(Apartment){tid,mode,1,0};result=0;}
 pthread_mutex_unlock(&c->mutex);return result;
}
int s26_query(Core*c,uint32_t tid,uint32_t*out){if(!c||!out)return 0;pthread_mutex_lock(&c->mutex);out[0]=out[1]=0;for(unsigned i=0;i<32;i++)if(c->image.threads[i].tid==tid){out[0]=c->image.threads[i].mode;out[1]=c->image.threads[i].count;break;}pthread_mutex_unlock(&c->mutex);return 1;}
int s26_uninitialize(Core*c,uint32_t tid){if(!c)return 0;int result=0;pthread_mutex_lock(&c->mutex);for(unsigned i=0;i<32;i++){Apartment*a=&c->image.threads[i];if(a->tid==tid&&a->count){if(!--a->count)memset(a,0,sizeof(*a));result=1;break;}}pthread_mutex_unlock(&c->mutex);return result;}
size_t s26_size(void){return sizeof(Image);}
int s26_export(Core*c,void*p,size_t n){if(!c||!p||n!=sizeof(Image))return 0;pthread_mutex_lock(&c->mutex);memcpy(p,&c->image,n);pthread_mutex_unlock(&c->mutex);return 1;}
int s26_import(Core*c,const void*p,size_t n){
 if(!c||!p||n!=sizeof(Image))return 0;Image im;memcpy(&im,p,n);if(im.magic!=0x434f4d32||im.version!=1)return 0;
 for(unsigned i=0;i<32;i++){Apartment*a=&im.threads[i];if(a->reserved||a->mode&~2u||(!a->tid&&(a->count||a->mode))||(a->tid&&!a->count))return 0;for(unsigned j=0;j<i;j++)if(a->tid&&a->tid==im.threads[j].tid)return 0;}
 pthread_mutex_lock(&c->mutex);c->image=im;pthread_mutex_unlock(&c->mutex);return 1;
}
