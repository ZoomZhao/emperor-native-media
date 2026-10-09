/* Platform-independent own semantics, separately tested on self-authored inputs. */
#include "stage70_runtime.h"
#define CF 1u
#define PF 4u
#define AF 16u
#define ZF 64u
#define SF 128u
#define OF 2048u
#define COND (CF|PF|AF|ZF|SF|OF)
uint32_t s70_mask(unsigned bits){return bits==32?UINT32_MAX:(1u<<bits)-1;}
uint32_t s70_reg(S70CPU*c,unsigned reg,unsigned bits,unsigned shift){return(c->r[reg]>>shift)&s70_mask(bits);}
void s70_setreg(S70CPU*c,unsigned reg,unsigned bits,unsigned shift,uint32_t v){uint32_t mask=s70_mask(bits)<<shift;c->r[reg]=(c->r[reg]&~mask)|((v<<shift)&mask);}
static S70Region *find(S70Memory*m,uint32_t address,unsigned bits,unsigned permission){
    unsigned n=bits/8;uint64_t end=(uint64_t)address+n;
    if(end>UINT64_C(0x100000000)){m->fault_address=address;return NULL;}
    for(size_t i=0;i<m->count;i++){S70Region*r=m->regions+i;if(address>=r->low&&end<=(uint64_t)r->low+r->size&&(r->permissions&permission))return r;}
    m->fault_address=address;return NULL;
}
int s70_read(S70Memory*m,uint32_t address,unsigned bits,uint32_t *v){S70Region*r=find(m,address,bits,1);if(!r)return S70_FAULT;*v=0;for(unsigned i=0;i<bits/8;i++)*v|=(uint32_t)r->bytes[address-r->low+i]<<(8*i);return S70_OK;}
int s70_write(S70Memory*m,uint32_t address,unsigned bits,uint32_t v){S70Region*r=find(m,address,bits,2);if(!r)return S70_FAULT;for(unsigned i=0;i<bits/8;i++)r->bytes[address-r->low+i]=(uint8_t)(v>>(8*i));m->writes++;return S70_OK;}
/* op0=add,1=sub,2=logic,3=inc,4=dec. Arithmetic flags use operand width. */
void s70_flags(S70CPU*c,uint32_t a,uint32_t b,uint32_t result,unsigned bits,unsigned op){
    uint32_t mask=s70_mask(bits),sign=1u<<(bits-1);a&=mask;b&=mask;result&=mask;
    uint32_t f=(result?0:ZF)|((result&sign)?SF:0),low=result&255,parity=0;
    for(unsigned i=0;i<8;i++)parity^=(low>>i)&1;if(!parity)f|=PF;
    if(op==2){uint32_t retained=c->flags&AF;c->flags=(c->flags&~COND)|f|retained;c->defined_flags=(c->defined_flags|COND)&~AF;return;}
    int sub=op==1||op==4;
    if(sub?(a<b):((uint64_t)a+b>mask))f|=CF;
    if((a^b^result)&16)f|=AF;
    if((sub?((a^b)&(a^result)):((~(a^b))&(a^result)))&sign)f|=OF;
    if(op==3||op==4)f=(f&~CF)|(c->flags&CF);
    c->flags=(c->flags&~COND)|f;c->defined_flags|=(op==3||op==4)?(COND&~CF):COND;
}
int s70_stos(S70CPU*c,S70Memory*m,unsigned bits,int repeat){
    if(c->es_base)return S70_SEGMENT;
    uint32_t count=repeat?c->r[1]:1,step=bits/8;
    while(count){
        if(!c->budget)return S70_BUDGET;
        int status=s70_write(m,c->r[7],bits,c->r[0]);if(status)return status;
        c->budget--;c->r[7]+=(c->flags&0x400)?(uint32_t)-step:step;
        count--;if(repeat)c->r[1]=count;
    }
    return S70_OK;
}
int s70_condition(uint32_t f,unsigned q){int c=!!(f&CF),p=!!(f&PF),z=!!(f&ZF),s=!!(f&SF),o=!!(f&OF);switch(q){case 0:return o;case 1:return!o;case 2:return c;case 3:return!c;case 4:return z;case 5:return!z;case 6:return c||z;case 7:return!c&&!z;case 8:return s;case 9:return!s;case 10:return p;case 11:return!p;case 12:return s!=o;case 13:return s==o;case 14:return z||s!=o;case 15:return!z&&s==o;default:return 0;}}
