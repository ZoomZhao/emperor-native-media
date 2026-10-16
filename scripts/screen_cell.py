"""Exact screen/capture/lattice registration; no texture identity or game input."""
from dataclasses import dataclass
from fractions import Fraction
import numpy as np


def _number(value):
    if isinstance(value,(bool,np.bool_)) or not isinstance(value,(int,float,np.integer,np.floating,Fraction)):
        raise ValueError('explicit finite numeric coordinate required')
    try:q=Fraction(str(value))
    except (ValueError,OverflowError):raise ValueError('explicit finite numeric coordinate required')from None
    if abs(q)>1_000_000:raise ValueError('coordinate budget')
    return q


def _integers(values,count,bound=1_000_000):
    if not isinstance(values,(list,tuple,np.ndarray))or len(values)!=count:
        raise ValueError('fixed integer coordinate extent required')
    if any(not isinstance(v,(int,np.integer))or isinstance(v,(bool,np.bool_))or abs(int(v))>bound for v in values):
        raise ValueError('bounded integer coordinates required')
    return tuple(map(int,values))


@dataclass(frozen=True)
class CaptureSpace:
    """Quartz global-point frame of the full window, including its title bar."""
    frame_points: tuple
    capture_pixels: tuple
    scale: int

    def __post_init__(self):
        if not isinstance(self.frame_points,(list,tuple,np.ndarray))or len(self.frame_points)!=4:raise ValueError('four window-frame coordinates required')
        frame=tuple(_number(v)for v in self.frame_points);pixels=_integers(self.capture_pixels,2,4096)
        if (not isinstance(self.scale,(int,np.integer))or isinstance(self.scale,(bool,np.bool_))
                or not 1<=self.scale<=8 or min(frame[2:])<=0 or min(pixels)<=0
                or pixels!=(frame[2]*self.scale,frame[3]*self.scale)):
            raise ValueError('measured equal-axis integer capture scale required')
        object.__setattr__(self,'frame_points',frame);object.__setattr__(self,'capture_pixels',pixels)
        object.__setattr__(self,'scale',int(self.scale))

    def screen_to_pixel(self,point):
        if not isinstance(point,(list,tuple,np.ndarray))or len(point)!=2:raise ValueError('two screen coordinates required')
        point=tuple(_number(v)for v in point);x,y,w,h=self.frame_points
        if not (x<=point[0]<x+w and y<=point[1]<y+h):raise ValueError('screen point outside recorded window')
        pixel=((point[0]-x)*self.scale,(point[1]-y)*self.scale)
        if any(v.denominator!=1 for v in pixel):raise ValueError('subpixel conversion is unknown; no rounding')
        return tuple(map(int,pixel))

    def pixel_to_screen(self,point):
        point=_integers(point,2);w,h=self.capture_pixels
        if not(0<=point[0]<w and 0<=point[1]<h):raise ValueError('capture pixel outside recorded window')
        return tuple(self.frame_points[i]+Fraction(point[i],self.scale)for i in range(2))


def _lattice(phase,basis):
    phase=_integers(phase,2)
    if not isinstance(basis,(list,tuple,np.ndarray))or len(basis)!=2:raise ValueError('two lattice basis rows required')
    a,b=_integers(basis[0],2,4096);c,d=_integers(basis[1],2,4096);det=a*d-b*c
    if not det:raise ValueError('independent measured lattice axes required')
    return phase,(a,b,c,d),det


def cell_center(cell,phase,basis):
    i,j=_integers(cell,2,4096);p,(a,b,c,d),_= _lattice(phase,basis)
    return (p[0]+a*i+b*j,p[1]+c*i+d*j)


def require_cell_center(point,phase,basis):
    point=_integers(point,2);p,(a,b,c,d),det=_lattice(phase,basis);x,y=point[0]-p[0],point[1]-p[1]
    ij=(Fraction(d*x-b*y,det),Fraction(a*y-c*x,det))
    if any(v.denominator!=1 for v in ij):raise ValueError('point is not an integer lattice centre; no snapping')
    return _integers(tuple(map(int,ij)),2,4096)


def _bounds(bounds,width,height):
    x0,y0,x1,y1=_integers(bounds,4)
    if not(0<=x0<x1<=width and 0<=y0<y1<=height):raise ValueError('complete fixed image bounds required')
    return x0,y0,x1,y1


def register_hover_cell(before,hover,cancelled,spaces,mouse_global,phase,basis,cells,probe_bounds,world_bounds):
    """Identify one complete public diamond on one fixed measured lattice.

    Uses raw before/hover differences, complete cancellation and outside-probe
    controls. No phase/scale/orientation fit, tint threshold, sprite or MAP lookup.
    A mouse point may be at a cell edge; it is never silently used as its centre.
    """
    if not isinstance(spaces,(list,tuple))or len(spaces)!=3 or any(not isinstance(s,CaptureSpace)for s in spaces)or any(s!=spaces[0]for s in spaces[1:]):
        raise ValueError('recorded capture geometry changed or is unknown')
    space=spaces[0];width,height=space.capture_pixels;images=list(map(np.asarray,(before,hover,cancelled)))
    if any(im.dtype!=np.uint8 or im.shape!=(height,width,3)for im in images):raise ValueError('matching bounded raw RGB captures required')
    before,hover,cancelled=images;px0,py0,px1,py1=_bounds(probe_bounds,width,height);wx0,wy0,wx1,wy1=_bounds(world_bounds,width,height)
    if not(wx0<=px0<px1<=wx1 and wy0<=py0<py1<=wy1):raise ValueError('probe must lie within fixed world bounds')
    changed=np.any(before!=hover,axis=2);outside=changed[wy0:wy1,wx0:wx1].copy();outside[py0-wy0:py1-wy0,px0-wx0:px1-wx0]=False
    if np.any(outside):raise ValueError('outside-probe controls changed; cause unknown')
    if not np.array_equal(before[wy0:wy1,wx0:wx1],cancelled[wy0:wy1,wx0:wx1]):raise ValueError('cancelled world differs; content/camera state unknown')
    if not isinstance(cells,(list,tuple))or not 1<=len(cells)<=128:raise ValueError('bounded predeclared calibration cells required')
    cells=[_integers(c,2,4096)for c in cells]
    if len(set(cells))!=len(cells):raise ValueError('duplicate calibration cells')
    logical=np.zeros((40,78),bool)
    for y in range(40):
        edge=38-2*y if y<20 else 2*y-40;logical[y,edge:78-edge]=True
    scale=space.scale;diamond=np.repeat(np.repeat(logical,scale,0),scale,1);mouse=space.screen_to_pixel(mouse_global);hits=[]
    for cell in cells:
        cx,cy=cell_center(cell,phase,basis);x0,y0=cx-39*scale,cy-20*scale;x1,y1=x0+78*scale,y0+40*scale
        if not(px0<=x0<x1<=px1 and py0<=y0<y1<=py1):continue
        expected=np.zeros((py1-py0,px1-px0),bool);expected[y0-py0:y1-py0,x0-px0:x1-px0]=diamond
        if np.array_equal(expected,changed[py0:py1,px0:px1]):hits.append((cell,(cx,cy),(x0,y0,x1,y1)))
    if len(hits)!=1:raise ValueError('no unique complete hover footprint on fixed lattice; no refit')
    cell,center,bounds=hits[0];x0,y0,x1,y1=bounds;mx,my=mouse
    if not(x0<=mx<x1 and y0<=my<y1 and diamond[my-y0,mx-x0]):raise ValueError('hover footprint does not contain the recorded mouse point')
    return {'relative_screen_cell':list(cell),'center_physical':list(center),'mouse_physical':list(mouse),
            'mouse_is_center':mouse==center,'footprint_bounds':list(bounds),'footprint_pixels':int(diamond.sum()),
            'outside_probe_changed_pixels':0,'cancelled_world_changed_pixels':0,'lattice_refitted':False,
            'source_identity_required':False,'absolute_MAP_cell_proved':False,'absence_of_occlusion_proved':False}


def compare_fixed_sprite(before,after,decoded_bgra,center,space,phase,basis,world_bounds):
    """Test frozen whole-sprite predicates; background identity is not an input.

    Alpha0/255 gives two explicit predictions: every opaque source pixel appears,
    and source-over-observed-before reproduces the entire canvas. A disagreement
    is returned as failure data, never a reason to remove pixels or refit a source.
    """
    from reference_compare import bottom_base_canvas_bounds
    if not isinstance(space,CaptureSpace):raise ValueError('recorded capture space required')
    require_cell_center(center,phase,basis)
    width,height=space.capture_pixels;before,after,source=map(np.asarray,(before,after,decoded_bgra))
    if any(a.dtype!=np.uint8 or a.shape!=(height,width,3)for a in (before,after)):
        raise ValueError('complete raw before/after RGB frames required')
    if source.dtype!=np.uint8 or source.ndim!=3 or source.shape[2]!=4 or source.shape[1]!=78 or not 40<=source.shape[0]<=4096:
        raise ValueError('declared single-cell BGRA canvas required')
    if np.any((source[:,:,3]!=0)&(source[:,:,3]!=255)):raise ValueError('partial-alpha mixing formula is unknown')
    if np.any(source[:,:,:3]>source[:,:,3,None])or not np.any(source[:,:,3]==255):raise ValueError('nonempty native premultiplied source required')
    wx0,wy0,wx1,wy1=_bounds(world_bounds,width,height)
    x0,y0,x1,y1=bottom_base_canvas_bounds(center,(78,source.shape[0]),(78,40),space.scale)
    if not(wx0<=x0<x1<=wx1 and wy0<=y0<y1<=wy1):raise ValueError('full prediction canvas must be visible inside fixed world bounds')
    rgb=np.repeat(np.repeat(source[:,:,2::-1],space.scale,0),space.scale,1)
    paint=np.repeat(np.repeat(source[:,:,3]==255,space.scale,0),space.scale,1)
    expected=before[y0:y1,x0:x1].copy();expected[paint]=rgb[paint];actual=after[y0:y1,x0:x1]
    bad=np.any((actual>>3)!=(rgb>>3),axis=2)&paint;canvas_bad=np.any((actual>>3)!=(expected>>3),axis=2)
    raw_bad=np.any(actual!=rgb,axis=2)&paint;raw_canvas_bad=np.any(actual!=expected,axis=2)
    def measured(mask,wrong,raw_wrong):
        total=int(mask.sum());count=int(wrong.sum())
        return {'equal_pixels':total-count,'different_pixels':count,'denominator':total,
                'passed':count==0,'raw_different_pixels':int(raw_wrong.sum())}
    return {'canvas_bounds':[x0,y0,x1,y1],'RGB_bits':5,'source_alpha_used_as_declared':True,
            'opaque_prediction':measured(paint,bad,raw_bad),
            'source_over_before_canvas_prediction':measured(np.ones(paint.shape,bool),canvas_bad,raw_canvas_bad),
            'opaque_mismatch_positions_local':np.argwhere(bad).tolist(),
            'canvas_mismatch_positions_local':np.argwhere(canvas_bad).tolist(),
            'background_source_match_required':False,'comparison_failure_retained':True,
            'absolute_no_occlusion_claimed':False,'engine_selected_ID_proved':False}


def validate_final_click(before,hover,cancelled,final_hover,frozen,contexts,journal):
    """Pure evidence gate; never sends input or extrapolates an unobserved point.

    Requires complete centred hover/cancel/reselect/final-hover evidence at the
    exact frozen click point. Contexts are before, hover, cancelled, final_hover,
    dispatch, from one monotonic producer. Any pointer/camera epoch change or
    unjournalled action invalidates this proof. The actual guarded sender must
    call this with its freshly sampled dispatch context, not reuse a result.
    """
    stages=('before','hover','cancelled','final_hover','dispatch')
    try:
        if not isinstance(contexts,list)or len(contexts)!=5:raise ValueError('five final-click contexts required')
        if not isinstance(frozen,dict)or not isinstance(journal,dict):raise ValueError('final-click evidence required')
        identity_keys=('owner','window','frame','capture_pixels','scale','camera_epoch','pointer_epoch','clock_id')
        if any(key not in frozen for key in (*identity_keys,'id','cell','center','click_global','phase','basis','probe_bounds','world_bounds')):
            raise ValueError('missing frozen final-click evidence')
        if not isinstance(frozen['id'],str)or not frozen['id']or not isinstance(frozen['clock_id'],str)or not frozen['clock_id']:
            raise ValueError('nonempty frozen/clock identity required')
        owner=frozen['owner'];owner_keys=('pid','uid','birth_seconds','birth_micros','executable')
        if not isinstance(owner,dict)or set(owner)!=set(owner_keys):raise ValueError('complete process birth/path identity required')
        if any(type(owner[k])is not int or owner[k]<0 for k in owner_keys[:-1])or owner['pid']==0 or not isinstance(owner['executable'],str)or not owner['executable'].startswith('/'):
            raise ValueError('complete process birth/path identity required')
        if type(frozen['window'])is not int or frozen['window']<=0:raise ValueError('window identity required')
        for key in ('camera_epoch','pointer_epoch'):
            if type(frozen[key])is not int or frozen[key]<0:raise ValueError('measured camera/pointer epoch required')
        space=CaptureSpace(frozen['frame'],frozen['capture_pixels'],frozen['scale'])
        cell=_integers(frozen['cell'],2,4096);center=_integers(frozen['center'],2)
        if cell_center(cell,frozen['phase'],frozen['basis'])!=center:raise ValueError('frozen cell/centre differs from fixed lattice')
        if space.screen_to_pixel(frozen['click_global'])!=center:raise ValueError('frozen click is not the frozen centre')
        times=[];serials=[]
        for stage,context in zip(stages,contexts):
            if not isinstance(context,dict)or context['stage']!=stage or context['freeze_id']!=frozen['id']:
                raise ValueError('context is not bound to this frozen trial')
            if context['identity_frame_front_verified']is not True:raise ValueError('current owner/frame/front evidence missing')
            if any(context[key]!=frozen[key]for key in identity_keys):raise ValueError('owner/frame/scale/camera/pointer context changed')
            if CaptureSpace(context['frame'],context['capture_pixels'],context['scale'])!=space or any(type(context[k])is not int for k in ('window','camera_epoch','pointer_epoch')):
                raise ValueError('measured final-click context required')
            if space.screen_to_pixel(context['cursor_global'])!=center:raise ValueError('observed pointer differs from final click point')
            t=context['monotonic_ns'];serial=context['event_serial']
            if type(t)is not int or t<=0 or type(serial)is not int or serial<0:raise ValueError('monotonic time/event serial required')
            times.append(t);serials.append(serial)
        if any(b<=a for a,b in zip(times,times[1:]))or times[-1]-times[0]>30_000_000_000 or times[-1]-times[-2]>250_000_000:
            raise ValueError('expired or unordered final-click registration')
        if serials!=[serials[0],serials[0]+1,serials[0]+2,serials[0]+3,serials[0]+3]:
            raise ValueError('intervening or missing final-click action')
        if journal['complete']is not True or journal['clock_id']!=frozen['clock_id'] or journal['freeze_id']!=frozen['id']:
            raise ValueError('complete same-clock action journal required')
        events=journal['events']
        if not isinstance(events,list)or len(events)!=3:raise ValueError('exact select/cancel/reselect journal required')
        for i,(event,kind)in enumerate(zip(events,('select_road_tool','cancel_tool','select_road_tool'))):
            if (event['kind']!=kind or any(type(event[k])is not int for k in ('serial','monotonic_ns','pointer_epoch','camera_epoch')) or event['serial']!=serials[i+1]
                    or not times[i]<event['monotonic_ns']<times[i+1]
                    or event['pointer_epoch']!=frozen['pointer_epoch'] or event['camera_epoch']!=frozen['camera_epoch']
                    or space.screen_to_pixel(event['cursor_global'])!=center):
                raise ValueError('tool/cancel action moved point/camera or lacks complete evidence')
        args=(before,hover,cancelled,[space]*3,frozen['click_global'],frozen['phase'],frozen['basis'],[cell],frozen['probe_bounds'],frozen['world_bounds'])
        measured=register_hover_cell(*args)
        final=register_hover_cell(before,final_hover,cancelled,*args[3:])
        for result in (measured,final):
            if not result['mouse_is_center']or result['mouse_physical']!=list(center)or result['center_physical']!=list(center)or result['relative_screen_cell']!=list(cell):
                raise ValueError('final preview point/cell is not the frozen centre')
    except (KeyError,TypeError,IndexError)as error:
        raise ValueError('missing or malformed final-click evidence')from error
    return {'passed':True,'frozen_id':frozen['id'],'cell':list(cell),'center_physical':list(center),
            'preview_point_equals_click_point':True,'mouse_is_center':True,'frozen_cell_confirmed':True,
            'cancel_reselect_point_unchanged':True,'final_preview_age_ns':times[-1]-times[-2],
            'background_source_match_required':False,'input_sent':False,'reusable_click_permit':False}
