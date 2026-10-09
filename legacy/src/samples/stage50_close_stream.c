#include "stage44_stream.c"
/* A stream is main-thread serviced; source PCM is never in the device FIFO.
 * Caller must first stop the real AudioUnit with the checked stage37 contract. */
int s50_stream_diag(State*s,uint64_t*out){
 if(!s||!out||s->im.magic!=0x5334344d||s->im.version!=1)return -1;uint32_t a[16];if(s37_stats(s->audio,a))return -2;
 Image*i=&s->im;uint64_t v[]={i->handle,i->generation,i->status,i->queued,i->frames,i->consumed,s->file!=NULL,s->pcm!=NULL,a[0],a[2],a[3],a[4],(uint64_t)a[6]|((uint64_t)a[7]<<32),(uint64_t)a[8]|((uint64_t)a[9]<<32)};memcpy(out,v,sizeof(v));return 0;
}
int s50_close_quiescent(State*s,uint32_t h,int32_t*out_status){
 if(!s||!out_status)return -1;*out_status=0;
 if(!h)return 0;Image*i=&s->im;
 if(h!=i->handle||!i->generation||i->generation>65535||h!=(0x5f000000u|(i->generation<<8)|1))return -2;
 uint32_t a[16];if(s37_stats(s->audio,a))return -3;
 if((a[0]!=0&&a[0]!=3)||a[2]||a[3]||a[4])return -20;
 if(!s->file||!s->pcm||!i->frames)return -21;
 if(s44_service(s))return -22;
 i->status=8;OSStatus status=ExtAudioFileDispose(s->file);*out_status=status;
 if(status)return -25;
 s->file=NULL;free(s->pcm);s->pcm=NULL;
 uint32_t generation=i->generation+1;memset(i,0,sizeof(*i));i->magic=0x5334344d;i->version=1;i->generation=generation;i->last_clock=clock_frames(s);
 return 0;
}
