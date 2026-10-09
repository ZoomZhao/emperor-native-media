#include "stage14_messages.h"
#ifndef CLIENT_FUNCTION
#error CLIENT_FUNCTION required
#endif
uint32_t CLIENT_FUNCTION(s14_messages *s,const char *p,uint32_t *e) {return s14_message_register(s,p,e);}
