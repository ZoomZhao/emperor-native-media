#ifndef S23_BRIDGE_H
#define S23_BRIDGE_H
#include <stdint.h>
typedef struct {int w,h,pitch,fmt,alive,locked,block;uint32_t guest;unsigned char *pixels;} Surface;
typedef struct {int x,y,w,h;} S23Rect;
Surface *s23_create(int w,int h,int pitch,int format);
int s23_free(Surface*);int s23_lock(Surface*,uint32_t*,int*);int s23_unlock(Surface*);
int s23_blt(Surface*,int,int,Surface*,S23Rect,int,uint32_t);
int s23_bltfast(Surface*,int,int,Surface*,S23Rect,int,uint32_t);
Surface *s23_frame(unsigned);
const unsigned char*s23_frame_initial(unsigned);
int s23_format(Surface*,uint32_t masks[4]);
int s23_verify(const char*,const unsigned char**,int*,int*,int*);
int s23_initialize(void);
uint64_t s23_native_offset(void);
#endif
