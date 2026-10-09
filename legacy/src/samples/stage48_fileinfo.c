#include "stage17_file.c"
#include <sys/mount.h>
/* Observed caller consumes only FILETIME LastWrite, offsets20/24.
 * Other fields expose actual APFS metadata; NTFS serial/attributes unproved. */
int s48_file_info(s17_file *s,uint32_t *out){
 if(!s||!out)return 0;struct stat st;struct statfs fs;uint64_t c,a,w;if(!file_valid(s,&st)||fstatfs(s->fd,&fs)||!file_ticks(st.st_birthtimespec,&c)||!file_ticks(st.st_atimespec,&a)||!file_ticks(st.st_mtimespec,&w))return 0;
 uint32_t attrs=((st.st_mode&0222)?0:1)|((st.st_flags&UF_HIDDEN)?2:0);if(!attrs)attrs=128;uint64_t size=st.st_size,index=st.st_ino;uint32_t v[13]={attrs,(uint32_t)c,(uint32_t)(c>>32),(uint32_t)a,(uint32_t)(a>>32),(uint32_t)w,(uint32_t)(w>>32),(uint32_t)fs.f_fsid.val[0],(uint32_t)(size>>32),(uint32_t)size,(uint32_t)st.st_nlink,(uint32_t)(index>>32),(uint32_t)index};memcpy(out,v,sizeof(v));return 1;
}
