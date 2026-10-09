/* Own native wall/boot clock bridge to measured fixed-Wine structure/units.
 * System time is live. Non-DST +08:00 guest profile is separate from host or
 * personal timezone settings; neither is changed by this implementation.
 */
#include "stage13_clock_profile.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
static int nanoseconds(clockid_t id,uint64_t *out) {
    struct timespec t;if(clock_gettime(id,&t)||t.tv_sec<0)return 0;
    *out=(uint64_t)t.tv_sec*1000000000+(uint64_t)t.tv_nsec;return 1;
}
int s13_clock_from_unix(uint64_t ns,int local,void *output,size_t size) {
    if(!output||size!=16)return 0;
    time_t seconds=(time_t)(ns/1000000000);
    if(local)seconds-=S13_TIMEZONE_BIAS_MINUTES*60;
    struct tm fields;if(!gmtime_r(&seconds,&fields)||fields.tm_year+1900<0||fields.tm_year+1900>65535)return 0;
    uint16_t values[8]={(uint16_t)(fields.tm_year+1900),(uint16_t)(fields.tm_mon+1),(uint16_t)fields.tm_wday,(uint16_t)fields.tm_mday,(uint16_t)fields.tm_hour,(uint16_t)fields.tm_min,(uint16_t)fields.tm_sec,(uint16_t)((ns%1000000000)/1000000)};
    memcpy(output,values,16);return 1;
}
int s13_clock_get(uint32_t kind,void *output,size_t size,uint32_t *retval,uint32_t *error) {
    *retval=0;uint64_t ns,value;
    if(kind==4) {
        if(!output||size!=172)return 0;memcpy(output,s13_tz,172);*retval=S13_TIMEZONE_RETURN;*error=S13_TIMEZONE_LAST_ERROR;return 1;
    }
    if(kind==5) {
        if(!output||size!=8)return 0;value=S13_CLOCK_FREQUENCY;memcpy(output,&value,8);*retval=1;return 1;
    }
    if(kind==6||kind==7) {
        if(!nanoseconds(CLOCK_MONOTONIC,&ns))return 0;
        if(kind==7) {*retval=(uint32_t)(ns/1000000);return size==0;}
        if(!output||size!=8)return 0;value=ns/(1000000000/S13_CLOCK_FREQUENCY);memcpy(output,&value,8);*retval=1;return 1;
    }
    if(!nanoseconds(CLOCK_REALTIME,&ns))return 0;
    if(kind==1||kind==2)return s13_clock_from_unix(ns,kind==1,output,size);
    if(kind==3) {if(!output||size!=8)return 0;value=ns/100+UINT64_C(116444736000000000);memcpy(output,&value,8);return 1;}
    return 0;
}
