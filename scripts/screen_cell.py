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
