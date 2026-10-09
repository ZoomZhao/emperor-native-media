/* Own isolated guest-session registry. No HWND, OS message dispatch or GUI.
 * Finite measured ASCII identifier scope. Capacity/unknown input is REFUSED,
 * not a fabricated Windows failure. Message numbers belong to this session,
 * never replay the captured Wine server's numeric allocation history. */
#include "stage14_messages.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
enum {CAPACITY=128,MAGIC=0x4d533134,VERSION=1};
typedef struct {uint32_t id,length;char name[256];} record;
typedef struct {uint32_t magic,version,capacity,count;record rows[CAPACITY];} image;
struct s14_messages {pthread_mutex_t lock;image data;};
static int normalize(const char *p,char out[256]) {
    if(!p)return -1;
    for(unsigned i=0;i<256;i++) {
        unsigned char c=(unsigned char)p[i];if(!c) {out[i]=0;return (int)i;}
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'))return -1;
        out[i]=(char)(c>='A'&&c<='Z'?c+32:c);
    }
    return -1;
}
s14_messages *s14_messages_new(void) {
    s14_messages *s=calloc(1,sizeof(*s));if(!s)return NULL;
    if(pthread_mutex_init(&s->lock,NULL)) {free(s);return NULL;}
    s->data.magic=MAGIC;s->data.version=VERSION;s->data.capacity=CAPACITY;return s;
}
void s14_messages_delete(s14_messages *s) {pthread_mutex_destroy(&s->lock);free(s);}
uint32_t s14_message_register(s14_messages *s,const char *p,uint32_t *error) {
    char name[256]={0};int n=normalize(p,name);
    if(n<0)return UINT32_C(0xfffffffe);
    if(!n) {*error=87;return 0;}
    pthread_mutex_lock(&s->lock);
    for(unsigned i=0;i<s->data.count;i++)if(!strcmp(s->data.rows[i].name,name)) {
        uint32_t id=s->data.rows[i].id;pthread_mutex_unlock(&s->lock);return id;
    }
    if(s->data.count==CAPACITY) {pthread_mutex_unlock(&s->lock);return UINT32_C(0xfffffffe);}
    unsigned i=s->data.count++;record *r=&s->data.rows[i];r->id=0xc000+i;r->length=(uint32_t)n;memcpy(r->name,name,(size_t)n+1);
    uint32_t id=r->id;pthread_mutex_unlock(&s->lock);return id;
}
size_t s14_messages_size(void) {return sizeof(image);}
uint32_t s14_messages_export(s14_messages *s,void *p,size_t n) {
    if(n!=sizeof(image))return 0;pthread_mutex_lock(&s->lock);memcpy(p,&s->data,n);pthread_mutex_unlock(&s->lock);return 1;
}
uint32_t s14_messages_import(s14_messages *s,const void *p,size_t n) {
    if(n!=sizeof(image))return 0;image d;memcpy(&d,p,n);
    if(d.magic!=MAGIC||d.version!=VERSION||d.capacity!=CAPACITY||d.count>CAPACITY)return 0;
    for(unsigned i=0;i<CAPACITY;i++) {
        record *r=&d.rows[i];
        if(i>=d.count) {record empty={0};if(memcmp(r,&empty,sizeof(empty)))return 0;continue;}
        if(r->id!=0xc000+i||!r->length||r->length>255||r->name[r->length])return 0;
        char normalized[256]={0};int len=normalize(r->name,normalized);
        if(len!=(int)r->length||memcmp(normalized,r->name,256))return 0;
        for(unsigned j=0;j<i;j++)if(!strcmp(r->name,d.rows[j].name))return 0;
    }
    pthread_mutex_lock(&s->lock);s->data=d;pthread_mutex_unlock(&s->lock);return 1;
}
