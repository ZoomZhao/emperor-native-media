/* Authored fixed-input ASCII INI reader. No filesystem writes or registry. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>
typedef struct {char section[128],key[128],value[512];} entry;
typedef struct {entry e[64];char sections[8][128];size_t count,nsec;} ini;
static int ascii(const char *p){if(!p)return 1;size_t n=strnlen(p,512);if(n==512)return 0;for(size_t i=0;i<n;i++)if((unsigned char)p[i]>127)return 0;return 1;}
static char *trim(char *p){while(*p==' '||*p=='\t')p++;size_t n=strlen(p);while(n&&(p[n-1]==' '||p[n-1]=='\t'||p[n-1]=='\r'))p[--n]=0;return p;}
static int parse(const char *body,size_t n,ini *a){if(!body||n>8192||memchr(body,0,n))return 0;for(size_t i=0;i<n;i++)if((unsigned char)body[i]>127)return 0;char copy[8193],section[128]={0};memcpy(copy,body,n);copy[n]=0;memset(a,0,sizeof(*a));char *ctx,*line=strtok_r(copy,"\n",&ctx);for(;line;line=strtok_r(NULL,"\n",&ctx)){line=trim(line);if(!*line||*line==';')continue;size_t z=strlen(line);if(line[0]=='['&&z>1&&line[z-1]==']'){line[z-1]=0;char *s=trim(line+1);if(strlen(s)>=128||a->nsec==8)return 0;strcpy(section,s);strcpy(a->sections[a->nsec++],s);continue;}char *eq=strchr(line,'=');if(!eq||!section[0])return 0;*eq=0;char *key=trim(line),*val=trim(eq+1);z=strlen(val);if(z>=2&&((val[0]=='"'&&val[z-1]=='"')||(val[0]=='\''&&val[z-1]=='\''))){val[z-1]=0;val++;}if(strlen(key)>=128||strlen(val)>=512||a->count==64)return 0;entry *e=&a->e[a->count++];strcpy(e->section,section);strcpy(e->key,key);strcpy(e->value,val);}return 1;}
static const char *lookup(ini *a,const char *section,const char *key){for(size_t i=0;i<a->count;i++)if(!strcasecmp(section,a->e[i].section)&&!strcasecmp(key,a->e[i].key))return a->e[i].value;return NULL;}
int s22_string(const char *body,size_t n,const char *section,const char *key,const char *fallback,char *out,size_t cap,uint32_t *copied,uint32_t *error){ini a;if(!parse(body,n,&a)||!ascii(section)||!ascii(key)||!ascii(fallback)||cap>512||!out||!copied||!error)return 0;*copied=0;*error=0;if(!cap)return 1;
 if(section&&key){const char *v=lookup(&a,section,key);size_t z;if(v)z=strlen(v);else{v=fallback?fallback:"";z=strlen(v);while(z&&v[z-1]==' ')z--;}if(z>=cap)z=cap-1;if(z)memcpy(out,v,z);out[z]=0;*copied=(uint32_t)z;return 1;}
 char list[8192];size_t at=0;if(!section){for(size_t i=0;i<a.nsec;i++){size_t z=strlen(a.sections[i])+1;memcpy(list+at,a.sections[i],z);at+=z;}}else for(size_t i=0;i<a.count;i++)if(!strcasecmp(section,a.e[i].section)){size_t z=strlen(a.e[i].key)+1;memcpy(list+at,a.e[i].key,z);at+=z;}
 if(!at){out[0]=0;return 1;}if(cap==1){out[0]=0;if(section)*error=87;return 1;}if(at+1>cap){size_t z=cap-2;if(z)memcpy(out,list,z);out[z]=0;*copied=(uint32_t)z;return 1;}memcpy(out,list,at);out[at]=0;*copied=(uint32_t)at;return 1;
}
int s22_integer(const char *body,size_t n,const char *section,const char *key,uint32_t fallback,uint32_t *value,uint32_t *error){ini a;if(!parse(body,n,&a)||!section||!key||!ascii(section)||!ascii(key)||!value||!error)return 0;const char *p=lookup(&a,section,key);*value=p?(uint32_t)strtoul(p,NULL,0):fallback;*error=0;return 1;}
