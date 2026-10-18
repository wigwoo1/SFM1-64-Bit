import hashlib,json,re,struct
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll'))
begin,end=0x10a0e901,0x10a0ee17;cursor=begin;spans=[]
with Path('work/ifm-disassembly.txt').open() as source:
 previous=None
 for line in source:
  match=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if match:
   if previous:
    address,data=previous;assert address==cursor;at=image.offset(address);assert image.b[at:at+len(data)]==data
    spans.append({'address':hex(address),'bytes':data.hex()});cursor+=len(data);previous=None
   address=int(match[1],16)
   if address>=end:break
   if address>=begin:previous=(address,bytes.fromhex(match[2]))
  elif previous:
   continuation=re.fullmatch(r'\s+((?:[0-9A-F]{2}\s*)+)',line)
   if continuation:previous=(previous[0],previous[1]+bytes.fromhex(continuation[1]))
assert cursor==end,(hex(cursor),hex(end))
report={'binary_sha256':hashlib.sha256(image.b).hexdigest(),'routine':'10a0e901','verified_instruction_spans':spans,'entry_arguments':'double low lanes xmm0/xmm1; result double low lane xmm0','range':'10a0e901..10a0ee17, last RET10a0ee16','dependencies':'Packed-double SSE arithmetic and read-only lookup tables; no CALL instructions seen in verified range','limits':'Algorithm inferred to be atan2 from callers and quadrant handling; exact implementation and table extent not reconstructed. All branch targets and exceptional dependency reachability must be validated before arbitrary input probing.'}
branches=[];calls=[];indirect=[]
with Path('work/ifm-disassembly.txt').open() as source:
 for line in source:
  match=re.match(r'\s*([0-9A-F]{8}):',line)
  if not match:continue
  address=int(match[1],16)
  if address>=end:break
  if address<begin:continue
  if re.search(r'\bcall\s',line):calls.append(line.strip())
  jump=re.search(r'\b(j[a-z]+)\s+([0-9A-F]{8})\b',line)
  if jump:branches.append({'source':hex(address),'kind':jump[1],'target':hex(int(jump[2],16))})
  elif re.search(r'\bj[a-z]+\s',line):indirect.append(line.strip())
assert not calls and not indirect
starts={span['address'] for span in spans}
assert all(begin<=int(branch['target'],16)<end and branch['target'] in starts for branch in branches)
report['verified_direct_branches']=branches;report['calls']=calls;report['indirect_branches']=indirect
report['limits']='Direct control flow bounded to verified instruction starts; no calls/indirect branches. Read-only table extents, FP environment and exact polynomial reconstruction remain unverified. atan2 identity inferred from caller/quadrant context.'
loads=[]
with Path('work/ifm-disassembly.txt').open() as source:
 for line in source:
  match=re.match(r'\s*([0-9A-F]{8}):',line)
  if not match:continue
  address=int(match[1],16)
  if address>=end:break
  if address<begin:continue
  memory=re.search(r'(xmmword|qword|mmword) ptr ds:\[([0-9A-F]{8})h\]',line)
  if memory:
   target=int(memory[2],16);width=16 if memory[1]=='xmmword' else 8
   offset=image.offset(target);assert image.offset(target+width-1)==offset+width-1
   loads.append({'instruction':hex(address),'address':hex(target),'width':width,'bytes':image.b[offset:offset+width].hex()})
windows=[]
for base,lastOffset in [(0x10dfdfc0,48),(0x10dfe000,48),(0x10dfe040,48),(0x10dfe080,48),(0x10dfd540,163*16)]:
 first=base;last=base+lastOffset+15
 offset=image.offset(first);assert image.offset(last)==offset+(last-first)
 windows.append({'base':hex(base),'maximum_read_end':hex(last),'verified_file_bytes':last-first+1,'window_sha256':hashlib.sha256(image.b[offset:offset+last-first+1]).hexdigest()})
report['file_backed_absolute_reads']=loads;report['conservative_indexed_windows']=windows
report['index_bounds']='EDX MOVMSKPD gives0..3 then doubles: read offsets0..48. Main xmm3 sign is cleared by ANDPD highword7fff, then MINSD clamps to highword4040. Original masked-highword guard admits3fa0..43f0; clamped word3fa0..4040 minus3f9e plus lowbit0..1 gives2..163, then doubles and scales8.'
report['limits']='Direct branches/read windows bounded and file-backed. Main indexed window bounds all guarded reads; logical table ownership/entry semantics remain unrecovered. Exact polynomial reconstruction, FP environment and execution parity remain unverified; atan2 identity remains inferred.'
Path('outputs/sfm64-angle-double-helper-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('angle_double_helper_spans='+str(len(spans)))
