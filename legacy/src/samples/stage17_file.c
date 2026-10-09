/* Own readonly file backing and metadata, no Windows ACL/archive-bit claim. */
#include "stage17_volume.c"
#include "stage13_clock_profile.h"
#include <time.h>
typedef struct {int fd;uint64_t device,inode,size;} s17_file;
static int file_valid(s17_file *s,struct stat *st) {return s&&!fstat(s->fd,st)&&S_ISREG(st->st_mode)&&(uint64_t)st->st_dev==s->device&&(uint64_t)st->st_ino==s->inode&&(uint64_t)st->st_size==s->size;}
s17_file *s17_file_new(const char *path) {
 int fd=open(path,O_RDONLY|O_NOFOLLOW);if(fd<0)return NULL;struct stat st;if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0) {close(fd);return NULL;}
 s17_file *s=calloc(1,sizeof(*s));if(!s) {close(fd);return NULL;}s->fd=fd;s->device=st.st_dev;s->inode=st.st_ino;s->size=st.st_size;return s;
}
void s17_file_delete(s17_file *s) {if(s) {close(s->fd);free(s);}}
int s17_file_read(s17_file *s,uint64_t offset,void *buffer,size_t capacity,size_t *received) {
 struct stat st;if(!buffer||!received||capacity>2*1024*1024||offset>INT64_MAX||!file_valid(s,&st))return 0;
 ssize_t n=pread(s->fd,buffer,capacity,(off_t)offset);if(n<0)return 0;*received=(size_t)n;return 1;
}
static int file_ticks(struct timespec t,uint64_t *out) {
 if(t.tv_sec<-11644473600LL||t.tv_nsec<0||t.tv_nsec>=1000000000)return 0;
 *out=((uint64_t)(t.tv_sec+11644473600LL))*10000000+(uint64_t)t.tv_nsec/100;return 1;
}
int s17_file_info(s17_file *s,void *output,size_t capacity) {
 struct stat st;uint64_t v[6];if(capacity!=sizeof(v)||!output||!file_valid(s,&st))return 0;
 v[0]=s->size;if(!file_ticks(st.st_birthtimespec,&v[1])||!file_ticks(st.st_atimespec,&v[2])||!file_ticks(st.st_mtimespec,&v[3]))return 0;v[4]=s->device;v[5]=s->inode;memcpy(output,v,sizeof(v));return 1;
}
int s17_file_basic_attributes(s17_file *s,uint32_t *output) {
 struct stat st;if(!output||!file_valid(s,&st))return 0;
 uint32_t flags=0;if(!(st.st_mode&0222))flags|=1;if(st.st_flags&UF_HIDDEN)flags|=2;
 /* Fixed input is a regular, non-dot file. No DOS archive/ACL/compression. */
 *output=flags?flags:0x80;return 1;
}
/* Only valid FILETIME values; observed old Wine overflow bug is refused. */
int s17_time_convert(uint32_t kind,uint64_t ticks,void *output,size_t capacity) {
 if(!output||ticks>=UINT64_C(0x8000000000000000))return 0;
 if(kind==1) {if(capacity!=8)return 0;uint64_t local=ticks-(int64_t)S13_TIMEZONE_BIAS_MINUTES*60*10000000LL;memcpy(output,&local,8);return 1;}
 if(kind!=2||capacity!=16)return 0;time_t sec=(time_t)(ticks/10000000)-11644473600LL;struct tm t;if(!gmtime_r(&sec,&t)||t.tm_year+1900<1601||t.tm_year+1900>65535)return 0;
 uint16_t v[8]={(uint16_t)(t.tm_year+1900),(uint16_t)(t.tm_mon+1),(uint16_t)t.tm_wday,(uint16_t)t.tm_mday,(uint16_t)t.tm_hour,(uint16_t)t.tm_min,(uint16_t)t.tm_sec,(uint16_t)((ticks%10000000)/10000)};memcpy(output,v,16);return 1;
}
int s17_identifier_equal(const char *a,const char *b,uint32_t *equal) {
 if(!a||!b||!equal)return 0;size_t na=strnlen(a,64),nb=strnlen(b,64);if(!na||!nb||na>=64||nb>=64)return 0;
 const char *texts[2]={a,b};size_t lengths[2]={na,nb};
 for(size_t t=0;t<2;t++)for(size_t i=0;i<lengths[t];i++) {unsigned char c=(unsigned char)texts[t][i];if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))return 0;}
 *equal=na==nb&&!memcmp(a,b,na);return 1;
}
