#include "stage17_file.c"
/* Adopt only an already validated openat FD; never reopen a pathname. */
s17_file *s49_file_from_fd(int input){
 int fd=dup(input);if(fd<0)return NULL;struct stat st;
 if(fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size<0){close(fd);return NULL;}
 s17_file *s=calloc(1,sizeof(*s));if(!s){close(fd);return NULL;}
 s->fd=fd;s->device=st.st_dev;s->inode=st.st_ino;s->size=st.st_size;return s;
}
