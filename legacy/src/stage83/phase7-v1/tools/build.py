"""Compile only reviewed task-authored event and pure surface implementations."""
from core import *
import subprocess,re
base=ROOT/'stage83/repair-v2/tools/unlock_headless.c'
surface=base.read_text()
lock=next(l for l in (ROOT/'samples/stage23_bridge.c').read_text().splitlines()if l.startswith('int s23_lock('))
lock29=next(l for l in (ROOT/'samples/stage30_dd.m').read_text().splitlines()if l.startswith('int s29_lock('))
addon=r'''
int p7_sync(void*p,uint32_t id,const void*bytes,size_t n){S30DD*c=p;DObj*o=c?obj(c,id):NULL;if(!o||o->kind!=2||!bytes||n!=(size_t)o->pitch*o->h)return 0;memcpy(c->surfaces[id-1]->pixels,bytes,n);return 1;}
int p7_lock(void*p,uint32_t id,uint32_t guest){S30DD*c=p;DObj*o=c?obj(c,id):NULL;if(!o||o->kind!=2)return 0;c->surfaces[id-1]->guest=guest;return s29_lock(p,id);}
/* The reviewed s23 32-bit path, including clip order and overlap snapshot.
 * 16-bit conversion requires original SR code and is refused before any store. */
int p7_blt32(void*p,uint32_t dst,int dx,int dy,uint32_t src,const int32_t*r){
 S30DD*c=p;DObj*d=c?obj(c,dst):NULL,*s=c?obj(c,src):NULL;
 if(!d||!s||d->kind!=2||s->kind!=2||!r||d->locked||s->locked||d->fmt!=32||s->fmt!=32)return 0;
 int64_t rw=(int64_t)r[2]-r[0],rh=(int64_t)r[3]-r[1];
 if(rw<0||rh<0||rw>8192||rh>8192||r[0]<-8192||r[0]>8192||r[1]<-8192||r[1]>8192||dx<-8192||dx>8192||dy<-8192||dy>8192)return 0;
 int sx=r[0],sy=r[1],w=(int)rw,h=(int)rh;if(!w||!h)return 1;
 if(sx<0){dx-=sx;w+=sx;sx=0;}if(sy<0){dy-=sy;h+=sy;sy=0;}
 if(dx<0){sx-=dx;w+=dx;dx=0;}if(dy<0){sy-=dy;h+=dy;dy=0;}
 if(sx+w>(int)s->w)w=(int)s->w-sx;if(sy+h>(int)s->h)h=(int)s->h-sy;
 if(dx+w>(int)d->w)w=(int)d->w-dx;if(dy+h>(int)d->h)h=(int)d->h-dy;if(w<=0||h<=0)return 1;
 size_t row=(size_t)w*4,n=row*h;unsigned char*copy=malloc(n);if(!copy)return 0;
 for(int y=0;y<h;y++)memcpy(copy+y*row,c->surfaces[src-1]->pixels+(sy+y)*s->pitch+sx*4,row);
 for(int y=0;y<h;y++)memcpy(c->surfaces[dst-1]->pixels+(dy+y)*d->pitch+dx*4,copy+y*row,row);
 free(copy);return 1;
}
'''
surface=surface.replace('int s29_unlock(',lock+'\n'+lock29+'\nint s29_unlock(')
target=V/'tools/surfaces.c';target.write_text(surface+addon)
commands=[['/usr/bin/clang','-O2','-dynamiclib',str(ROOT/'samples/stage13_events.c'),'-o',str(V/'builds/events.dylib'),'-pthread'],['/usr/bin/clang','-O2','-Wall','-Wextra','-dynamiclib',str(target),'-o',str(V/'builds/surfaces.dylib')]]
rows=[]
for i,c in enumerate(commands):
 q=subprocess.run(c,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=30);(V/f'evidence/build-{i}.stdout').write_bytes(q.stdout);(V/f'evidence/build-{i}.stderr').write_bytes(q.stderr);rows.append({'command':c,'returncode':q.returncode});assert q.returncode==0
put(V/'evidence/build.json',{'rows':rows,'pure_native_sources':{str(p.relative_to(ROOT)):sha(p)for p in (ROOT/'samples/stage13_events.c',ROOT/'samples/stage13_events.h',base,target)},'library_pins':{str(p.relative_to(ROOT)):sha(p)for p in (V/'builds/events.dylib',V/'builds/surfaces.dylib')},'lock_unlock_functions_reused_verbatim':['s23_lock','s23_unlock','s29_lock','s29_unlock'],'Blt32_source':'reviewed s23 clipping order and snapshot copy, finite32-only version; 16bit SR conversion refused','original_SR_or_UI_functions_linked':False})
print(json.dumps({'status':'PURE_EVENTS_AND_SURFACES_BUILD_PASS','original_execution':0}))
