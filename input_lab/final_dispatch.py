# SPDX-License-Identifier: MIT
"""Single-attempt final-click coordinator and same-process native dry-run adapter.

No live sender or evidence producer is provided. Existing CLI final-road mode
refuses explicitly; preflight JSON is never converted to a click subprocess.
"""
import copy
import ctypes
import json
import hashlib
from pathlib import Path
import sys
import threading
import uuid
import numpy as np

_claimed_trials=set()
_claim_lock=threading.Lock()

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'scripts'))
from screen_cell import CaptureSpace,validate_final_click


class ExistingCLIAdapter:
    mode='unsupported-live'
    reason='existing CLI cannot retain a same-process final-preview/camera/pointer witness'


class FakeClickSender:
    """Records fake down/up attempts; has no OS event interface."""
    def __init__(self,fail_after_down=False):
        self.events=[];self.calls=0;self.fail_after_down=fail_after_down

    def __call__(self,point):
        self.calls+=1
        self.events.append({'kind':'left_down','point':list(point),'fake':True,'OS_posted':False})
        if self.fail_after_down:raise RuntimeError('authored failure after fake down')
        self.events.append({'kind':'left_up','point':list(point),'fake':True,'OS_posted':False})


class NativeDryRunAdapter:
    mode='dry-run'
    Gate=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_double,ctypes.c_double,ctypes.c_void_p,ctypes.POINTER(ctypes.c_int64))
    Send=ctypes.CFUNCTYPE(ctypes.c_int,ctypes.c_double,ctypes.c_double,ctypes.c_void_p)
    Clock=ctypes.CFUNCTYPE(ctypes.c_int64,ctypes.c_void_p)

    def __init__(self,library,native_fault='',request_point=None):
        library=Path(library)
        if not library.is_absolute()or not library.is_file():raise ValueError('explicit owned dry-run library required')
        self.library=ctypes.CDLL(str(library));self.calls=0
        self.native_fault=native_fault;self.request_point=request_point
        self.entry=self.library.run_dry_final
        self.entry.argtypes=[ctypes.c_char_p,self.Gate,self.Send,self.Clock,ctypes.c_void_p,ctypes.c_char_p,ctypes.c_size_t]
        self.entry.restype=ctypes.c_int

    def invoke(self,frozen,gate,sender,clock,audit):
        point=self.request_point if self.request_point is not None else frozen['click_global']
        x,y,w,h=frozen['frame'];request={**frozen['owner'],'window_id':frozen['window'],
            'frame_x':x,'frame_y':y,'frame_width':w,'frame_height':h,
            'normalized_x':(point[0]-x)/w,'normalized_y':(point[1]-y)/h}
        command=json.dumps({'mode':'dry-run','request':request,'native_fault':self.native_fault}).encode()
        def checked(x,y,user,requested):
            try:requested[0]=gate((x,y));return 0
            except BaseException as error:audit.append({'event':'validator_refused','reason':str(error)});return 1
        def emitted(x,y,user):
            audit.append({'event':'fake_sender_entered','point':[x,y]})
            try:sender((x,y));return 0
            except BaseException as error:audit.append({'event':'fake_sender_failed','reason':str(error)});return 1
        def sampled(user):
            try:
                value=clock()
                if type(value)is not int or not 0<value<2**63:raise ValueError('same-producer clock unavailable')
                return value
            except BaseException as error:audit.append({'event':'clock_refused','reason':str(error)});return -1
        check,emit,now=self.Gate(checked),self.Send(emitted),self.Clock(sampled)
        output=ctypes.create_string_buffer(16384);self.calls+=1
        code=self.entry(command,check,emit,now,None,output,len(output))
        result=json.loads(output.value)
        if code not in (0,1)or result['passed']!=(code==0):raise RuntimeError('native outcome unconfirmed; do not retry')
        return result


class FinalClickAttempt:
    """Consumes once before any native call, including refusals and uncertainty."""
    def __init__(self,images,frozen,contexts,journal):
        if len(images)!=4 or any(not isinstance(image,np.ndarray)or image.dtype!=np.uint8 or image.ndim!=3 or image.shape[2]!=3 or max(image.shape[:2])>4096 for image in images):
            raise ValueError('four bounded raw evidence images required')
        self.images=tuple(image.copy()for image in images)
        for image in self.images:image.setflags(write=False)
        self.frozen=copy.deepcopy(frozen);self.contexts=copy.deepcopy(contexts);self.journal=copy.deepcopy(journal)
        self.id=str(uuid.uuid4());self.audit=[];self.consumed=False
        self.cancelled=threading.Event();self.lock=threading.Lock()
        self._sealed_metadata=self._metadata_digest()
        self._sealed_images=tuple(hashlib.sha256(image.tobytes()).digest()for image in self.images)

    def _metadata_digest(self):
        return hashlib.sha256(json.dumps([self.frozen,self.contexts,self.journal],sort_keys=True,allow_nan=False,separators=(',',':')).encode()).digest()

    def _check_integrity(self):
        if self._metadata_digest()!=self._sealed_metadata or any(image.flags.writeable for image in self.images):
            raise ValueError('attempt evidence changed')
        if tuple(hashlib.sha256(image.tobytes()).digest()for image in self.images)!=self._sealed_images:
            raise ValueError('attempt pixels changed')

    def cancel(self):
        self.cancelled.set()

    def run(self,adapter,context_provider,sender,clock,after_first_validation=None):
        with self.lock:
            if self.consumed:
                self.audit.append({'event':'consumed_attempt_refused'})
                return {'status':'refused_consumed_attempt','native_calls_this_run':0,'sender_calls_this_run':0}
            self.consumed=True
        with _claim_lock:
            key=(self.frozen.get('clock_id'),self.frozen.get('id'))
            if any(not isinstance(value,str)or not value for value in key):
                self.audit.append({'event':'missing_trial_identity'})
                return {'status':'refused_missing_trial_identity','native_calls_this_run':0,'sender_calls_this_run':0}
            if key in _claimed_trials or len(_claimed_trials)>=4096:
                self.audit.append({'event':'frozen_trial_reuse_refused'})
                return {'status':'refused_reused_frozen_trial','native_calls_this_run':0,'sender_calls_this_run':0}
            _claimed_trials.add(key)
        self.audit.append({'event':'attempt_consumed','attempt_id':self.id})
        if adapter.mode!='dry-run':
            self.audit.append({'event':'unsupported_live_adapter','reason':adapter.reason})
            return {'status':'unsupported_live_adapter','native_calls_this_run':0,'sender_calls_this_run':0,'reason':adapter.reason}
        if self.cancelled.is_set():
            self.audit.append({'event':'cancelled_before_native_entry'})
            return {'status':'cancelled_before_native_entry','native_calls_this_run':0,'sender_calls_this_run':0}
        calls_before=adapter.calls;validations=0
        def gate(point):
            nonlocal validations
            if self.cancelled.is_set():raise ValueError('attempt cancelled')
            self._check_integrity()
            space=CaptureSpace(self.frozen['frame'],self.frozen['capture_pixels'],self.frozen['scale'])
            if space.screen_to_pixel(point)!=tuple(self.frozen['center']):raise ValueError('native request differs from frozen final point')
            # Native code invokes this again after its final OS guard snapshot.
            # Each call samples producer state; no saved verdict is reused.
            if validations>=2:raise ValueError('unexpected repeated validator callback')
            contexts=copy.deepcopy(self.contexts);contexts[-1]=copy.deepcopy(context_provider())
            validate_final_click(*self.images,self.frozen,contexts,self.journal)
            validations+=1;self.audit.append({'event':'validator_passed','sample':validations})
            if validations==1 and after_first_validation is not None:after_first_validation()
            if self.cancelled.is_set():raise ValueError('attempt cancelled')
            return self.contexts[-2]['monotonic_ns']
        try:
            result=adapter.invoke(self.frozen,gate,sender,clock,self.audit)
            result.update(attempt_id=self.id,validation_calls=validations,native_calls_this_run=adapter.calls-calls_before,
                          fake_events=copy.deepcopy(sender.events),original_input_sent=False,audit=copy.deepcopy(self.audit))
            return result
        except Exception as error:
            self.audit.append({'event':'native_outcome_unconfirmed','reason':str(error)})
            return {'status':'unconfirmed_no_retry','native_calls_this_run':adapter.calls-calls_before,
                    'sender_calls_this_run':None,'reason':str(error),'audit':copy.deepcopy(self.audit)}
