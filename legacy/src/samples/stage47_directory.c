#include "stage17_file.c"
int s47_directory_info(const char *path,uint64_t *v,uint32_t *attrs){
 if(!path||!v||!attrs)return 0;int fd=open(path,O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return 0;struct stat st;int ok=!fstat(fd,&st)&&S_ISDIR(st.st_mode);close(fd);if(!ok)return 0;
 v[0]=st.st_size;v[4]=st.st_dev;v[5]=st.st_ino;if(!file_ticks(st.st_birthtimespec,v+1)||!file_ticks(st.st_atimespec,v+2)||!file_ticks(st.st_mtimespec,v+3))return 0;
 *attrs=16|((st.st_mode&0222)?0:1)|((st.st_flags&UF_HIDDEN)?2:0);return 1;
}
