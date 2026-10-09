"""Own finite observation state. No game, emulator, native library or network imports."""
import copy

class WatchStop(Exception):
    def __init__(self, kind, **details):
        self.details = dict(kind=kind, **details)
        super().__init__(str(self.details))

class EmulationGate:
    """Process-local guard; wrapping the trusted engine does not run it."""
    def __init__(self): self.allowed = False; self.attempts = 0; self.calls = 0
    def wrap(self, fn):
        def guarded(*a, **kw):
            self.attempts += 1
            if not self.allowed: raise WatchStop('Emulation_disabled_before_run_or_during_restore54')
            self.calls += 1
            return fn(*a, **kw)
        return guarded

def initial_state():
    return dict(version=1, starting_phase=5, phase=5, entries=0,
                advances=[], tail_entries=0, tail_returns=0, frames=[],
                caller_zero_branch=False, caller_accepted=False,
                baseline_primary_sha256=None, frame_candidate=None,
                refresh_returns=0, observations=[], full_phase_cycle=False)

class Watch:
    MODES = ('restore', 'caller', 'frame', 'cycle')
    def __init__(self, mode, state=None):
        if mode not in self.MODES: raise ValueError('unknown observation mode')
        self.mode = mode
        self.state = copy.deepcopy(state if state is not None else initial_state())
        self.validate()

    def validate(self):
        s = self.state
        if set(s) != set(initial_state()) or s['version'] != 1 or s['starting_phase'] != 5:
            raise ValueError('unknown or reset observation state')
        if type(s['entries']) is not int or not 0 <= s['entries'] <= 128:
            raise ValueError('scheduler entry bound')
        if not 0 <= s['tail_returns'] <= s['tail_entries'] <= 1:
            raise ValueError('cycle tail bound')
        if not isinstance(s['advances'], list) or len(s['advances']) > 51:
            raise ValueError('productive phase bound')
        phase = 5
        wraps = 0
        for e in s['advances']:
            if e != dict(before=phase, after=(phase+1) % 51):
                raise ValueError('phase sequence is not the observed natural sequence')
            if phase == 50: wraps += 1
            phase = e['after']
        if phase != s['phase'] or len(s['advances']) > s['entries']:
            raise ValueError('phase/counter persistence mismatch')
        # A tail return precedes the enclosing scheduler return at the same PC.
        pending_wrap = any(f['kind'] == 'scheduler' and f['before'] == 50 and f.get('wrap_seen')
                           for f in s['frames'])
        if s['tail_returns'] != wraps and not (pending_wrap and s['tail_returns'] == wraps+1):
            raise ValueError('tail return is not tied to a productive phase wrap')
        if s['full_phase_cycle'] != (len(s['advances']) == 51 and s['tail_returns'] == 1 and phase == 5):
            raise ValueError('partial wrap mislabeled full cycle')
        for f in s['frames']:
            if f['kind'] not in ('scheduler', 'tail', 'refresh') or f['tid'] != 1:
                raise ValueError('unsupported pending observer frame')
            if not all(type(f[k]) is int for k in ('tid', 'sp', 'ret')):
                raise ValueError('observer frame ABI')
        if len(s['frames']) > 8 or len(s['observations']) > 512:
            raise ValueError('observation resource bound')
        if s['caller_accepted'] and not s['caller_zero_branch']:
            raise ValueError('caller return lacks its natural branch')

    def snapshot(self):
        self.validate()
        return copy.deepcopy(self.state)

    def note(self, kind, **details):
        if len(self.state['observations']) >= 512:
            raise WatchStop('Observation_capacity54')
        self.state['observations'].append(dict(kind=kind, **details))

    def original_event(self):
        if self.mode == 'restore': raise WatchStop('Original_execution_forbidden_during_restore54')

    def caller_branch(self, tid, pc):
        self.original_event()
        if self.mode != 'caller' or tid != 1 or pc != 0x401adc:
            raise WatchStop('Caller_branch_scope54', tid=tid, pc=hex(pc))
        self.state['caller_zero_branch'] = True
        self.note('caller_zero_AL_branch', pc=hex(pc), tid=tid)

    def caller_return(self, tid, pc, sp, screen, route):
        self.original_event()
        if (self.mode != 'caller' or tid != 1 or pc != 0x44a7e2 or sp != 0x700ffaa4
                or screen != 1 or route != 1 or not self.state['caller_zero_branch']):
            raise WatchStop('Caller_return_or_state_mismatch54', tid=tid, pc=hex(pc),
                            sp=hex(sp), screen=screen, route=route)
        self.state['caller_accepted'] = True
        self.note('caller_return', tid=tid, pc=hex(pc), sp=hex(sp), screen=screen)
        raise WatchStop('Caller_return_goal_stop54', caller_instruction_executed=False)

    def scheduler_entry(self, tid, sp, ret, phase, gate):
        self.original_event()
        s = self.state
        if self.mode not in ('frame', 'cycle') or not s['caller_accepted']:
            raise WatchStop('Scheduler_before_caller_or_outside_mode54')
        if len(s['advances']) >= 51 or s['entries'] >= 128 or s['tail_returns'] >= 1:
            raise WatchStop('Scheduler_cap_before_entry54', entries=s['entries'], advances=len(s['advances']))
        if tid != 1 or ret != 0x5371c4 or gate != 0 or phase != s['phase']:
            raise WatchStop('Scheduler_caller_gate_or_phase_mismatch54', tid=tid,
                            ret=hex(ret), gate=gate, phase=phase, expected_phase=s['phase'])
        if any(f['kind'] == 'scheduler' for f in s['frames']):
            raise WatchStop('Recursive_scheduler54')
        s['entries'] += 1
        s['frames'].append(dict(kind='scheduler', tid=tid, sp=sp, ret=ret, before=phase, wrap_seen=False))
        self.note('scheduler_entry', ordinal=s['entries'], tid=tid, sp=hex(sp), phase=phase)

    def wrap_jump(self, tid, sp, phase):
        self.original_event()
        frames = [f for f in self.state['frames'] if f['kind'] == 'scheduler']
        if len(frames) != 1 or (tid, sp, phase, frames[0]['before']) != (1, frames[0]['sp'], 0, 50):
            raise WatchStop('Natural_wrap_jump_mismatch54')
        frames[0]['wrap_seen'] = True
        self.note('original_wrap_tail_jump', tid=tid, sp=hex(sp))

    def tail_entry(self, tid, sp, ret, phase):
        self.original_event()
        s = self.state
        if s['tail_entries'] >= 1: raise WatchStop('Second_cycle_tail_forbidden54')
        frames = [f for f in s['frames'] if f['kind'] == 'scheduler']
        if (len(frames) != 1 or tid != 1 or sp != frames[0]['sp'] or ret != frames[0]['ret']
                or phase != 0 or not frames[0]['wrap_seen']):
            raise WatchStop('Tail_without_original_shared_stack_jump54')
        s['tail_entries'] += 1
        s['frames'].append(dict(kind='tail', tid=tid, sp=sp, ret=ret))
        self.note('tail_entry', tid=tid, sp=hex(sp), ret=hex(ret))

    def refresh_entry(self, tid, sp, ret, screen):
        self.original_event()
        if self.mode not in ('frame', 'cycle') or screen != 1: return
        if tid != 1: raise WatchStop('City_refresh_wrong_thread54')
        self.state['frames'].append(dict(kind='refresh', tid=tid, sp=sp, ret=ret, candidate=None))
        self.note('city_refresh_entry', tid=tid, sp=hex(sp), ret=hex(ret))

    def primary_present(self, tid, screen, event, original_primary_bytes_sha256):
        self.original_event()
        if self.mode not in ('frame', 'cycle') or screen != 1: return
        frames = [f for f in self.state['frames'] if f['kind'] == 'refresh' and f['tid'] == tid]
        if not frames: return
        if (event.get('source') != 'only original successful primary Blt pixels'
                or event.get('method') not in ('2.5', '2.7')
                or event.get('stats', {}).get('GPU_byte_mismatches') != 0
                or event.get('stats', {}).get('actual_drawable_presentations', 0) < 1
                or event.get('geometry', {}).get('client_w') != 1024
                or event.get('geometry', {}).get('client_h') != 768):
            raise WatchStop('Unproven_original_primary_present54')
        baseline = self.state['baseline_primary_sha256']
        if baseline is None: raise WatchStop('Original_primary_baseline_missing54')
        if original_primary_bytes_sha256 == baseline: return
        if not self.state['frame_candidate']:
            frames[-1]['candidate'] = copy.deepcopy(event)
            frames[-1]['candidate']['original_primary_bytes_sha256'] = original_primary_bytes_sha256
            frames[-1]['candidate']['semantic_city_content_reviewed'] = False

    def returns(self, tid, pc, sp, phase):
        self.original_event()
        s = self.state
        candidates = [f for f in s['frames'] if f['ret'] == pc and f['tid'] == tid]
        if not candidates: return
        # Only original tail-jump shares the scheduler stack; tail processed first.
        exact = [f for f in candidates if sp == f['sp']+4]
        if not exact:
            raise WatchStop('Observed_return_stack_mismatch54', pc=hex(pc), sp=hex(sp))
        order = {'tail': 0, 'scheduler': 1, 'refresh': 2}
        pause = None
        for f in sorted(exact, key=lambda q: order[q['kind']]):
            if f['kind'] == 'tail':
                if phase != 0: raise WatchStop('Tail_changed_phase_unexpectedly54', phase=phase)
                s['tail_returns'] += 1
                self.note('tail_return', pc=hex(pc), sp=hex(sp), phase=phase)
                pause = 'Finite_scheduler_cap_stop54'
            elif f['kind'] == 'scheduler':
                before = f['before']
                if phase == before:
                    if f['wrap_seen']: raise WatchStop('Wrap_without_phase_progress54')
                    self.note('scheduler_precheck_or_no_progress_return', phase=phase)
                elif phase == (before+1) % 51:
                    if before == 50 and (not f['wrap_seen'] or s['tail_returns'] != 1):
                        raise WatchStop('Wrap_without_real_tail_return54')
                    s['advances'].append(dict(before=before, after=phase))
                    s['phase'] = phase
                    self.note('productive_scheduler_return', before=before, after=phase,
                              ordinal=len(s['advances']), pc=hex(pc), sp=hex(sp))
                else:
                    raise WatchStop('Unexpected_original_phase_sequence54', before=before, after=phase)
                s['full_phase_cycle'] = len(s['advances']) == 51 and s['tail_returns'] == 1 and phase == 5
                if len(s['advances']) == 51 or s['entries'] == 128:
                    pause = 'Finite_scheduler_cap_stop54'
            else:
                s['refresh_returns'] += 1
                if f['candidate'] and not s['frame_candidate']:
                    s['frame_candidate'] = f['candidate']
                    self.note('first_fresh_city_mode_frame_candidate', pc=hex(pc), sp=hex(sp),
                              semantic_city_content_reviewed=False)
                    if self.mode == 'frame': pause = pause or 'Fresh_city_frame_candidate_stop54'
            s['frames'].remove(f)
        self.validate()
        if pause:
            raise WatchStop(pause, full_phase_cycle=s['full_phase_cycle'], phase=phase,
                            productive_advances=len(s['advances']), tail_returns=s['tail_returns'],
                            semantic_city_content_reviewed=False, caller_instruction_executed=False)
