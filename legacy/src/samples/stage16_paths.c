/* Own bounded Windows guest cwd, backed by an actual open POSIX directory.
 * A loader namespace alias is input; never write the game's startup globals.
 * Host process cwd is unchanged. Empty path yields measured INVALID_NAME;
 * unknown path resolution refuses rather than inventing successful chdir.
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
typedef struct {uint32_t magic,version;uint64_t device,inode;char guest[261];} image;
typedef struct {int fd;image data;} s16_paths;
static int valid(s16_paths *s) {struct stat st;return !fstat(s->fd,&st)&&S_ISDIR(st.st_mode)&&(uint64_t)st.st_dev==s->data.device&&(uint64_t)st.st_ino==s->data.inode;}
s16_paths *s16_paths_new(const char *guest,const char *host) {
 if(!guest||!host)return NULL;size_t n=strnlen(guest,261);if(n<3||n>260||guest[1]!=':'||guest[2]!='\\')return NULL;
 int fd=open(host,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return NULL;struct stat st;if(fstat(fd,&st)||!S_ISDIR(st.st_mode)) {close(fd);return NULL;}
 s16_paths *s=calloc(1,sizeof(*s));if(!s) {close(fd);return NULL;}s->fd=fd;s->data.magic=0x43573136;s->data.version=1;s->data.device=(uint64_t)st.st_dev;s->data.inode=(uint64_t)st.st_ino;memcpy(s->data.guest,guest,n+1);return s;
}
void s16_paths_delete(s16_paths *s) {close(s->fd);free(s);}
int s16_cwd_set(s16_paths *s,const char *path,uint32_t *value,uint32_t *error) {
 if(!path||!valid(s))return 0;
 if(!*path) {
  int fd=openat(s->fd,path,O_RDONLY|O_DIRECTORY);int e=errno;if(fd>=0) {close(fd);return 0;}if(e!=ENOENT)return 0;
  *value=0;*error=123;return 1;
 }
 if(strcmp(path,".")&&strcmp(path,s->data.guest))return 0;
 int fd=openat(s->fd,".",O_RDONLY|O_DIRECTORY);if(fd<0)return 0;struct stat st;int okay=!fstat(fd,&st)&&(uint64_t)st.st_dev==s->data.device&&(uint64_t)st.st_ino==s->data.inode;close(fd);if(!okay)return 0;*value=1;return 1;
}
int s16_cwd_get(s16_paths *s,uint32_t capacity,char *buffer,uint32_t *value,uint32_t *error) {
 (void)error;if(!valid(s))return 0;size_t n=strlen(s->data.guest);
 if(capacity<=n) {*value=(uint32_t)n+1;return 1;}
 if(!buffer)return 0;memcpy(buffer,s->data.guest,n+1);*value=(uint32_t)n;return 1;
}
size_t s16_paths_size(void) {return sizeof(image);}
int s16_paths_export(s16_paths *s,void *p,size_t n) {if(n!=sizeof(image)||!valid(s))return 0;memcpy(p,&s->data,n);return 1;}
int s16_paths_import(s16_paths *s,const void *p,size_t n) {return n==sizeof(image)&&valid(s)&&!memcmp(p,&s->data,n);}
/* Finite non-overlapping valid ASCII buffers only; no DBCS or SEH claim. */
int s16_string_copy(char *dest,size_t capacity,const char *src,size_t length,uint32_t count,size_t *written) {
 if(!dest||!src||!written||count>260||length>4096||src[length])return 0;
 for(size_t i=0;i<length;i++)if(!src[i]||(unsigned char)src[i]>=128)return 0;
 *written=0;if(!count)return 1;size_t n=length<count-1?length:count-1;
 if(capacity<n+1)return 0;memcpy(dest,src,n);dest[n]=0;*written=n+1;return 1;
}
int s16_string_length(const char *src,size_t length,uint32_t *value) {
 if(!src||!value||length>4096||src[length])return 0;
 for(size_t i=0;i<length;i++)if(!src[i]||(unsigned char)src[i]>=128)return 0;
 *value=(uint32_t)length;return 1;
}
