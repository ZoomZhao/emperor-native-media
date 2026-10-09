/* Own narrow profile adapter: measured EOC.ini-missing branch only. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
int s19_missing_profile(const char *windows_dir,const char *fallback,void *output,size_t capacity,uint32_t *copied,uint32_t *error) {
 if(!windows_dir||!output||!capacity||capacity>512||!copied||!error)return 0;
 DIR *d=opendir(windows_dir);if(!d)return 0;struct dirent *p;int found=0;
 while((p=readdir(d)))if(!strcasecmp(p->d_name,"EOC.ini")){found=1;break;}
 if(found){closedir(d);return 0;}
 int fd=openat(dirfd(d),"EOC.ini",O_RDONLY|O_NOFOLLOW);int e=errno;if(fd>=0)close(fd);closedir(d);
 if(fd>=0||e!=ENOENT)return 0;
 size_t n=fallback?strnlen(fallback,512):0;if(n>=512)return 0;
 for(size_t i=0;i<n;i++)if((unsigned char)fallback[i]>127)return 0;
 while(n&&fallback[n-1]==' ')n--;if(n>=capacity)n=capacity-1;
 if(n)memcpy(output,fallback,n);((char*)output)[n]=0;*copied=(uint32_t)n;*error=2;return 1;
}
int s19_missing_version_file(const char *game_dir) {
 DIR *d=opendir(game_dir);if(!d)return 0;struct dirent *p;int found=0;
 while((p=readdir(d)))if(!strcasecmp(p->d_name,"_wonapiversion.txt")){found=1;break;}
 if(found){closedir(d);return 0;}
 int fd=openat(dirfd(d),"_wonapiversion.txt",O_RDONLY|O_NOFOLLOW);int e=errno;if(fd>=0)close(fd);closedir(d);return fd<0&&e==ENOENT;
}
