/* Task-owned paths only: caller must enforce its local filesystem namespace. */
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
int s13_directory_create(const char *path,uint32_t *value,uint32_t *error) {
    if(!mkdir(path,0777)) {*value=1;return 1;}
    *value=0;
    switch(errno) {
        case EEXIST:*error=183;break;
        case ENOENT:*error=3;break;
        case EACCES:case EPERM:*error=5;break;
        case ENOSPC:*error=112;break;
        case ENAMETOOLONG:*error=206;break;
        default:return 0;
    }
    return 1;
}
int s13_disk_space(const char *path,uint32_t output[4],uint32_t *error) {
    (void)error;struct statvfs info;if(statvfs(path,&info))return 0;
    uint64_t cluster=info.f_frsize?info.f_frsize:info.f_bsize;
    if(!cluster||cluster%512||cluster/512>UINT32_MAX||info.f_bavail>UINT32_MAX||info.f_blocks>UINT32_MAX)return 0;
    output[0]=(uint32_t)(cluster/512);output[1]=512;output[2]=(uint32_t)info.f_bavail;output[3]=(uint32_t)info.f_blocks;return 1;
}
