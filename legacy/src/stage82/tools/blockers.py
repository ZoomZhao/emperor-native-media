import json,re,sys
sys.dont_write_bytecode=True
from common import ROOT,S,CORE,API,put
def main():
 c=json.loads((S/'configs/original.json').read_text());prior=json.loads((ROOT/'stage76/evidence/full-audit.json').read_text());nodes={r['pc']:r for r in c['instructions']};regs={int(r['PC'],0):r for r in json.loads((ROOT/'stage78/evidence/dynamic-domains.json').read_text())['register_IAT_sources']if r['all_CFG_paths_have_single_IAT_source']};api_sites=[];apirows=[]
 oldnames={n.lstrip('_')for n in json.loads((ROOT/'stage81/evidence/result.json').read_text())['unimplemented_Win32_API_symbols']};implemented={n for n,k in API.values()};group={}
 inventory=prior['expanded_named_APIs'];names={r['API']for r in inventory}
 for name in sorted({r['API']for r in regs.values()}-names):inventory.append({'API':name,'sites':[r['PC']for r in regs.values()if r['API']==name]})
 for r in inventory:
  name=r['API'].rsplit('.',1)[1]
  for p in r['sites']:
   pc=int(p,0);op=nodes[pc]['operand'];m=re.fullmatch(r'dword ptr \[(\d+)\]',op)
   if m:slot=int(m[1]);reg=8
   elif pc in regs:slot=int(regs[pc]['slot'],0);reg=CORE.index(regs[pc]['register'].upper())
   else:raise RuntimeError('UNREVIEWED_API_OPERAND '+p+' '+op)
   api_sites.append({'site':pc,'slot':slot,'reg':reg,'name':name})
  if name not in oldnames:continue
  if name in implemented:category='captured_single_thread_lock';status='IMPLEMENTED_CONDITIONALLY_AUTHORED_TEST_PENDING'
  elif any(x in name for x in ('String','CPInfo','WideChar','ByteTo','TimeZone','Environment')):category='locale_time_environment';status='BOUNDARY'
  elif name in ('LoadLibraryA','GetProcAddress'):category='loader_and_dynamic_User32';status='BOUNDARY'
  elif name in ('VirtualAlloc','VirtualFree'):category='CRT_virtual_memory';status='BOUNDARY'
  elif name in ('RaiseException','TerminateProcess','ExitProcess','GetCurrentProcess'):category='SEH_fatal_termination';status='BOUNDARY'
  else:category='CRT_files_and_process_metadata';status='BOUNDARY'
  row={'API':name,'sites':r['sites'],'category':category,'status':status,'necessary_on_this_natural_input':'UNOBSERVED; static reachability alone is not actual-path evidence','failure_policy':'STOP_WHOLE_ATTEMPT_NO_SUCCESS_STUB'};apirows.append(row);group.setdefault(category,[]).append(name)
 assert len(apirows)==23
 d=json.loads((ROOT/'stage79/evidence/call-domains.json').read_text());known={r['site']for r in json.loads((ROOT/'stage81/evidence/extended-domains.json').read_text())['rows']};unknown=[r for g in d['groups'].values()for r in g if int(r['PC'],0)not in known];assert len(unknown)==20
 put(S/'evidence/blockers.json',{'Win32_previous_total':23,'conditional_new_implementations':3,'remaining_Win32_boundaries':20,'groups':group,'APIs':apirows,'unknown_indirect_CALL_sites':unknown,'unknown_groups':{'virtual':11,'CRT_callbacks':6,'dynamic_User32':3},'SEH_dispatch_unwind':'UNIMPLEMENTED_FAIL','active_natural_input_available':False,'API_host_write_original_trace_equivalence':'not proved until new original differential','no_function_set_expansion':True})
 c['named_sites']=api_sites;c['native_driver']='stage82/builds/original-v3/native-driver';(S/'configs/original.json').write_text(json.dumps(c,indent=2)+'\n');print(json.dumps({'previous_Win32':23,'new_conditional_lock_APIs':3,'remaining_API_boundaries':20,'unknown_CALLs':20,'guarded_named_API_sites':len(api_sites)}))
if __name__=='__main__':main()
