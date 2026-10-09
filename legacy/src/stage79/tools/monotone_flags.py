"""Finite, monotone interprocedural arithmetic-flag provenance analysis.

Each flag is a powerset of original producer PCs and INCOMING, extended with
an absorbing TOP. None is temporary bottom (no known normal return), never a
reported proof of dead code. Unclosed no-return summaries are promoted to TOP
and solved again. Unknown control exits/calls/flag effects produce TOP.
"""
import collections
INCOMING='incoming'
TOP='unknown/top'
BOTTOM=None
TOP_STATE=tuple(frozenset([TOP])for _ in range(6))
ENTRY_STATE=tuple(frozenset([INCOMING])for _ in range(6))
def join_bit(a,b):return frozenset([TOP])if TOP in a or TOP in b else a|b
def join(a,b):
 if a is None:return b
 if b is None:return a
 return tuple(join_bit(x,y)for x,y in zip(a,b))
def leq_bit(a,b):return TOP in b or(TOP not in a and a<=b)
def leq(a,b):return a is None or b is not None and all(leq_bit(x,y)for x,y in zip(a,b))
def substitute(summary,state):
 if summary is None:return None
 return tuple(frozenset([TOP])if TOP in origins else join_bit(origins-{INCOMING},state[b])if INCOMING in origins else origins for b,origins in enumerate(summary))
def tarjan(graph):
 index={};low={};stack=[];active=set();components=[]
 def visit(v):
  index[v]=low[v]=len(index);stack.append(v);active.add(v)
  for w in sorted(graph[v]):
   if w not in index:visit(w);low[v]=min(low[v],low[w])
   elif w in active:low[v]=min(low[v],index[w])
  if low[v]==index[v]:
   component=[]
   while True:
    w=stack.pop();active.remove(w);component.append(w)
    if w==v:break
   components.append(sorted(component))
 for v in sorted(graph):
  if v not in index:visit(v)
 return components
class Solver:
 def __init__(self,nodes,entries):
  self.nodes=nodes;self.entries=set(entries);self.summaries={f:None for f in entries};self.states={};self.diagnostics=[];self.calls={f:set()for f in entries};self.predecessors=collections.defaultdict(set)
  self.evaluations=0;self.summary_updates=0;self.node_updates=0;self.no_return_promotions=[]
  for f in sorted(entries):
   seen=set();q=[f]
   while q:
    pc=q.pop()
    if pc in seen or pc not in nodes:continue
    seen.add(pc);r=nodes[pc]
    if r.get('local_callee')in self.entries:self.calls[f].add(r['local_callee']);self.predecessors[r['local_callee']].add(f)
    q+=r.get('successors',[])
  self.SCCs=tarjan(self.calls)
 def effect(self,r,state):
  out=list(state)
  for b in range(6):
   changed=False
   if r.get('defines',0)&(1<<b):out[b]=frozenset([r['pc']]);changed=True
   if r.get('undefined',0)&(1<<b):out[b]=frozenset([TOP]);changed=True
   if r.get('semantic_unknown'):out[b]=frozenset([TOP]);changed=True
   if changed and r.get('conditional_effect'):out[b]=join_bit(out[b],state[b])
  return tuple(out)
 def evaluate(self,entry):
  self.evaluations+=1;states={entry:ENTRY_STATE};q=collections.deque([entry]);queued={entry};returns=None;open_exits=[]
  while q:
   pc=q.popleft();queued.remove(pc)
   if pc not in self.nodes:
    returns=join(returns,TOP_STATE);open_exits.append(pc);continue
   r=self.nodes[pc];out=self.effect(r,states[pc])
   if r.get('local_callee')in self.entries:out=substitute(self.summaries[r['local_callee']],out)
   elif r.get('call_unknown'):out=TOP_STATE
   if out is None:continue # temporary solver bottom; promoted before reporting
   if r.get('normal_return'):returns=join(returns,out)
   if r.get('open_exit'):returns=join(returns,TOP_STATE);open_exits.append(pc)
   for successor in r.get('successors',[]):
    old=states.get(successor);new=join(old,out)
    assert leq(old,new)
    if new!=old:
     self.node_updates+=1;states[successor]=new
     if successor not in queued:q.append(successor);queued.add(successor)
  self.states[entry]=states
  for pc in open_exits:
   row=(entry,pc)
   if row not in self.diagnostics:self.diagnostics.append(row)
  return returns
 def drain(self,initial):
  q=collections.deque(initial);queued=set(initial)
  while q:
   f=q.popleft();queued.remove(f);old=self.summaries[f];computed=self.evaluate(f);new=join(old,computed)
   assert leq(old,new)
   if new!=old:
    self.summaries[f]=new;self.summary_updates+=1
    for caller in sorted(self.predecessors[f]):
     if caller not in queued:q.append(caller);queued.add(caller)
 def solve(self):
  # Tarjan emits callees before callers. Cross-SCC propagation still uses the
  # explicit reverse call graph; no scheduling order is required for soundness.
  self.drain([f for component in self.SCCs for f in component])
  unclosed=[f for f,s in self.summaries.items()if s is None]
  for f in unclosed:self.summaries[f]=TOP_STATE;self.no_return_promotions.append(f)
  if unclosed:self.drain(sorted(self.entries))
  # Re-evaluate final CFG states against the stable summaries, then verify the
  # post-fixpoint property. No iteration cap can yield a success result.
  for f in sorted(self.entries):
   computed=self.evaluate(f);assert leq(computed,self.summaries[f]),('not post-fixpoint',f)
  self.converged=True
  return self
 def statistics(self):
  tokens=len(self.nodes)+1
  # A non-TOP bit gains <=tokens origins, then at most one TOP promotion.
  return dict(converged=self.converged,domain='six finite provenance powersets + absorbing TOP + temporary return bottom',origin_token_bound=tokens,summary_height_bound_per_function=6*(tokens+1)+1,summary_update_upper_bound=len(self.entries)*(6*(tokens+1)+1),summary_updates=self.summary_updates,evaluations=self.evaluations,node_updates=self.node_updates,SCC_count=len(self.SCCs),recursive_SCCs=[c for c in self.SCCs if len(c)>1 or c[0]in self.calls[c[0]]],no_return_promoted_to_top=self.no_return_promotions,open_control_exits=self.diagnostics,postfixpoint_verified=True,unclosed_returns_never_reported_dead=True)
