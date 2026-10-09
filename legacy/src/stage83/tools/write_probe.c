/* Authored OS-write negative probe; no game object/context/code is linked. */
#include <errno.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char**argv){if(argc!=5)return 2;FILE*f=fopen(argv[1],"wbx");if(!f)return 3;fputs("inside-only\n",f);fclose(f);for(int i=2;i<5;i++){errno=0;f=fopen(argv[i],"wb");if(f){fputs("UNEXPECTED_ESCAPE\n",f);fclose(f);return 4;}if(errno!=EPERM&&errno!=EACCES)return 5;printf("C kernel write refused errno=%d\n",errno);}return 0;}
