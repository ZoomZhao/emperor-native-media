import re
def instrument(text):
 pointers={};output=[];loads=stores=index=0
 for line in text.splitlines():
  if line.startswith('define '):pointers={}
  m=re.fullmatch(r'(%\w+) = inttoptr i64 %\w+ to i(8|16|32)\*',line)
  if m:pointers[m[1]]=int(m[2])//8
  m=re.match(r'(?:%\w+ = )?(load|store) i(8|16|32)(?:, | )(.+)',line)
  if m:
   kind,bits,tail=m.groups();pointer=re.search(r'i\d+\* (%\w+)',tail)
   if pointer and pointer[1]in pointers:
    p=pointer[1];index+=1;n=pointers[p];cast='%s81_guard_'+str(index);output +=[cast+' = bitcast i'+bits+'* '+p+' to i8*','call void @stage72_guard(i8* '+cast+',i32 '+str(n)+',i32 '+str(2 if kind=='store'else 1)+')',line]
    if kind=='store':
     v=tail.split(',',1)[0];wide='%s81_value_'+str(index)
     if v.startswith('%'):output.append(wide+' = zext i'+bits+' '+v+' to i64');v=wide
     else:v=str(int(v)&((1<<int(bits))-1))
     output.append('call void @stage72_note_write(i8* '+cast+',i32 '+str(n)+',i64 '+v+')');stores+=1
    else:loads+=1
    continue
  output.append(line)
 if index:output+=['declare void @stage72_guard(i8*,i32,i32)','declare void @stage72_note_write(i8*,i32,i64)']
 return '\n'.join(output)+'\n',dict(scalar_load_guards=loads,scalar_store_guards=stores)
