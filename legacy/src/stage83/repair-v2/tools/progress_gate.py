"""Explicit natural entry/return state machine; cross-cycle defaults denied."""
class Limit(Exception):pass
class Progress:
 def __init__(self,c):
  self.c=c;self.phase=c['initial_phase'];self.advances=c['initial_advances'];self.entries=c['initial_entries'];self.tails=c['initial_tail_returns'];self.pending=None;self.tail_pending=False;self.write_count=0
  if c['cross_cycle_allowed']:raise Limit('CROSS_CYCLE_NOT_AUTHORIZED_BY_THIS_TOOL_VERSION')
  if not 0<=self.phase<51 or self.advances>c['maximum_total_advances']or self.entries>c['maximum_total_entries']or self.tails>c['maximum_total_tail_returns']:raise Limit('INVALID_INITIAL_PROGRESS')
 def event(self,kind,tid,pc,sp,phase,ret=None):
  c=self.c
  if tid!=1:raise Limit('WORKER_SCHEDULING_REFUSED')
  if kind=='entry':
   if pc!=c['scheduler_entry']or ret!=c['scheduler_return']or phase!=self.phase or self.pending is not None:raise Limit('NATURAL_SCHEDULER_ENTRY_MISMATCH')
   if self.entries>=c['maximum_total_entries']or self.advances>=c['maximum_total_advances']or self.tails>=c['maximum_total_tail_returns']:raise Limit('PROGRESS_CAP_BEFORE_ENTRY')
   self.entries+=1;self.pending=(phase,sp);return
  if kind=='tail':
   if pc!=c['tail_entry']or self.pending is None or self.pending[0]!=50 or phase!=0 or self.tails>=c['maximum_total_tail_returns']:raise Limit('TAIL_WITHOUT_NATURAL_WRAP')
   raise Limit('CROSS_CYCLE_REFUSED_BEFORE_TAIL')
  if kind=='return':
   if pc!=c['scheduler_return']or self.pending is None or sp!=self.pending[1]+4:raise Limit('NATURAL_SCHEDULER_RETURN_MISMATCH')
   before=self.pending[0]
   if phase==before:self.pending=None;return
   if before==50 or phase!=before+1:raise Limit('CROSS_CYCLE_OR_PHASE_SKIP_REFUSED')
   if self.advances>=c['maximum_total_advances']:raise Limit('ADVANCE_CAP_BEFORE_ACCEPT')
   self.advances+=1;self.phase=phase;self.pending=None;return
  raise Limit('UNKNOWN_PROGRESS_EVENT')
 def before_write(self,n):
  if n<1 or self.write_count>=self.c['maximum_writes']:raise Limit('WRITE_CAP_BEFORE_STORE')
  self.write_count+=1
 def snapshot(self):return dict(phase=self.phase,advances=self.advances,entries=self.entries,tail_returns=self.tails,pending=self.pending,cross_cycle_allowed=False,write_records=self.write_count)
