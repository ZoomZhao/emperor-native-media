/* Measured fixed-Wine startup/cleanup gateway. No guest socket, name lookup,
 * bind, listen, connection or packet operation is implemented or issued.
 * Verify the host's local unconnected-socket capability before admitting
 * startup. Actual network calls require their own verified adapter.
 */
#include "stage13_winsock.h"
#include "stage13_events.h"
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
uint32_t s13_wsa_startup(uint32_t *references,uint32_t version,void *output,size_t size,uint32_t *error) {
    (void)error;version&=0xffff;
    if(!version)return 10092;
    if(version!=0x101&&version!=0x202)return S13_REFUSED;
    if(!output)return 10014;
    if(size!=400||*references>=65535)return S13_REFUSED;
    int descriptor=socket(AF_INET,SOCK_STREAM,0);
    if(descriptor<0)return S13_REFUSED;
    if(close(descriptor))return S13_REFUSED;
    unsigned char *body=output;
    body[0]=(unsigned char)version;body[1]=(unsigned char)(version>>8);body[2]=2;body[3]=2;
    memcpy(body+4,"WinSock 2.0",12);memcpy(body+261,"Running",8);
    body[390]=0x80;body[391]=0;body[392]=0;body[393]=4;
    ++*references;return 0;
}
uint32_t s13_wsa_cleanup(uint32_t *references,uint32_t *error) {
    if(!*references) {*error=10093;return UINT32_MAX;}
    --*references;return 0;
}
