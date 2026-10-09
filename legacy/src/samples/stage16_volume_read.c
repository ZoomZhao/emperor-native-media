/* Own read-only live filesystem evidence. Not a Windows API adapter. */
#include <sys/attr.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
int main(int argc,char **argv) {
 if(argc!=2)return 2;int fd=open(argv[1],O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(fd<0)return 3;
 struct statfs fs;if(fstatfs(fd,&fs))return 4;long maximum=fpathconf(fd,_PC_NAME_MAX);
 struct attrlist query;memset(&query,0,sizeof(query));query.bitmapcount=ATTR_BIT_MAP_COUNT;query.volattr=ATTR_VOL_INFO|ATTR_VOL_CAPABILITIES;
 struct {uint32_t length;vol_capabilities_attr_t caps;} result;memset(&result,0,sizeof(result));int status=getattrlist(argv[1],&query,&result,sizeof(result),0);int error=errno;
 printf("{\"native_filesystem\":\"%s\",\"maximum_component_bytes\":%ld,\"getattrlist_status\":%d,\"failure_errno_only\":%d,\"format_caps\":%u,\"valid_format_caps\":%u,\"interface_caps\":%u,\"valid_interface_caps\":%u,\"Windows_adapter_implemented\":false}\n",fs.f_fstypename,maximum,status,status?error:0,result.caps.capabilities[0],result.caps.valid[0],result.caps.capabilities[1],result.caps.valid[1]);close(fd);return status?5:0;
}
