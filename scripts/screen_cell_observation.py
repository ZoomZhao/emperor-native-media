"""Replay explicit private hover evidence; never launches, captures or inputs UI."""
import os
if not os.environ.get('EMPEROR_GAMEDATA_ROOT'):
    print('{"skipped":true,"reason":"EMPEROR_GAMEDATA_ROOT unset"}')
    raise SystemExit(0)
from pathlib import Path
import argparse,json,sys


def replay(root,manifest):
    from PIL import Image
    import numpy as np
    from screen_cell import CaptureSpace,register_hover_cell,require_cell_center
    if Path(manifest['source_root']).resolve()!=root:raise ValueError('explicit source-root provenance differs')
    prior=json.loads(Path(manifest['prior_lattice']).read_text())
    if manifest['phase']!=prior['fixed_phase']or manifest['basis']!=prior['fixed_basis']:
        raise ValueError('lattice differs from prior measured evidence; no refit')
    images=[];spaces=[];identities=[]
    for key in ('before','hover','cancelled'):
        record=json.loads(Path(manifest[key]).read_text())
        if not record['same_owner_front_frame']:raise ValueError('capture identity/frame check not confirmed')
        with Image.open(record['capture'])as image:
            if max(image.size)>4096:raise ValueError('capture budget')
            spaces.append(CaptureSpace(tuple(record['frame']),image.size,manifest['measured_scale']))
            images.append(np.asarray(image.convert('RGB')))
        identities.append((record['owner'],record['window']))
    if any(i!=identities[0]for i in identities[1:]):raise ValueError('historical capture owner/window differs')
    request=json.loads(Path(manifest['move_request']).read_text());dispatch=json.loads(Path(manifest['move_dispatch']).read_text())
    owner,window=identities[0]
    if (request['kind']!='move' or not dispatch['input_sent']or dispatch['kind']!='guarded_cursor_warp_and_mouse_moved_only'
            or dispatch['pid']!=owner['pid']or dispatch['window_id']!=window or request['window_id']!=window
            or any(request[k]!=v for k,v in owner.items())):
        raise ValueError('recorded move does not belong to the capture owner/window')
    frame=tuple(request[k]for k in ('frame_x','frame_y','frame_width','frame_height'))
    if CaptureSpace(frame,spaces[0].capture_pixels,manifest['measured_scale'])!=spaces[0]:
        raise ValueError('move frame differs from capture frame')
    result=register_hover_cell(*images,spaces,dispatch['point'],manifest['phase'],manifest['basis'],
                               manifest['calibration_cells'],manifest['probe_bounds'],manifest['world_bounds'])
    try:require_cell_center(result['mouse_physical'],manifest['phase'],manifest['basis']);mouse_rejected=False
    except ValueError:mouse_rejected=True
    converted=spaces[0].pixel_to_screen(result['center_physical'])
    result.update(measured_capture_scale=spaces[0].scale,capture_pixels=list(spaces[0].capture_pixels),
                  recorded_frame=list(map(float,spaces[0].frame_points)),mouse_as_center_rejected=mouse_rejected,
                  future_center_screen_point=[int(v)if v.denominator==1 else str(v)for v in converted],
                  historical_context_only=True,source_or_background_match_used=False,original_inputs=0)
    return result


def replay_final(root,manifest):
    """Explicit final-click mode; historical measurement alone never suffices."""
    from PIL import Image
    import numpy as np
    from screen_cell import validate_final_click
    measured=replay(root,manifest)
    evidence=manifest['final_click'];images=[]
    if not measured['mouse_is_center']or measured['center_physical']!=evidence['frozen']['center']or measured['relative_screen_cell']!=evidence['frozen']['cell']:
        raise ValueError('recorded hover is not at the frozen final click centre')
    for index,key in enumerate(('before','hover','cancelled','final_hover')):
        record=json.loads(Path(manifest[key]).read_text())
        context=evidence['contexts'][index]
        with Image.open(record['capture'])as image:
            if max(image.size)>4096:raise ValueError('capture budget')
            if (record['final_click_context']!=context or not record['same_owner_front_frame']or context['owner']!=record['owner']or context['window']!=record['window']
                    or context['frame']!=record['frame']or context['capture_pixels']!=list(image.size)
                    or context['scale']!=manifest['measured_scale']):
                raise ValueError('final-click context differs from actual capture metadata')
            images.append(np.asarray(image.convert('RGB')))
    frozen=evidence['frozen']
    if any(frozen[key]!=manifest[key]for key in ('phase','basis','probe_bounds','world_bounds')):
        raise ValueError('final-click lattice/control bounds differ from frozen manifest')
    return validate_final_click(*images,frozen,evidence['contexts'],evidence['journal'])


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--manifest',type=Path,required=True);parser.add_argument('--final-click',action='store_true');args=parser.parse_args()
    root=Path(os.environ['EMPEROR_GAMEDATA_ROOT'])
    if not root.is_absolute()or not root.is_dir():parser.error('existing absolute source root required')
    try:result=(replay_final if args.final_click else replay)(root.resolve(),json.loads(args.manifest.read_text()))
    except (ValueError,KeyError,TypeError,IndexError,OSError)as error:
        print(json.dumps({'passed':False,'reason':str(error)}));return 1
    print(json.dumps({'passed':True,'mode':'final-click'if args.final_click else 'measurement','registration':result}));return 0

if __name__=='__main__':raise SystemExit(main())
