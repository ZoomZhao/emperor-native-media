/* Own finite consumer projection, never a complete Windows filesystem claim.
 * Known original AfxFullPath consumer reads exactly mask 0x6. Other bits/volume
 * serial/name are not implemented; reject consumers requesting another mask.
 * APFS UTF-8-on-disk is documented by Apple and checked in local regression.
 */
#include "stage16_paths.c"
#include <sys/attr.h>
#include <sys/mount.h>
#include <limits.h>
typedef struct {uint32_t maximum,flags,format,valid_format,interfaces,valid_interfaces;} s17_volume;
int s17_volume_read(s16_paths *s,const char *root,uint32_t consumer_mask,void *output,size_t capacity) {
 if(!s||!output||capacity!=sizeof(s17_volume)||consumer_mask!=6||!valid(s))return 0;
 if(root&&strcmp(root,"C:\\")&&strcmp(root,"c:\\"))return 0;
 if(s->data.guest[0]!='C')return 0;
 struct statfs fs;if(fstatfs(s->fd,&fs)||strcmp(fs.f_fstypename,"apfs"))return 0;
 long maximum=fpathconf(s->fd,_PC_NAME_MAX);if(maximum<1||maximum>UINT32_MAX)return 0;
 char path[PATH_MAX];if(fcntl(s->fd,F_GETPATH,path))return 0;
 struct attrlist query;memset(&query,0,sizeof(query));query.bitmapcount=ATTR_BIT_MAP_COUNT;query.volattr=ATTR_VOL_INFO|ATTR_VOL_CAPABILITIES;
 struct {uint32_t length;vol_capabilities_attr_t caps;} result;memset(&result,0,sizeof(result));
 if(getattrlist(path,&query,&result,sizeof(result),0)||!(result.caps.valid[VOL_CAPABILITIES_FORMAT]&VOL_CAP_FMT_CASE_PRESERVING))return 0;
 s17_volume value={(uint32_t)maximum,4,result.caps.capabilities[0],result.caps.valid[0],result.caps.capabilities[1],result.caps.valid[1]};
 if(result.caps.capabilities[VOL_CAPABILITIES_FORMAT]&VOL_CAP_FMT_CASE_PRESERVING)value.flags|=2;
 memcpy(output,&value,sizeof(value));return 1;
}
