from pathlib import Path
import json,re,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
start,end=0x10198b40,0x1019fb40
refs=[]
for row in rows:
 addresses=[int(v,16) for v in re.findall(r'(?<![0-9A-F])([0-9A-F]{8})h',row['instruction'])]
 if any(start<=v<end for v in addresses):
  a=int(row['address'],16);raw=bytes.fromhex(row['bytes']);assert raw==im.b[im.offset(a):im.offset(a)+len(raw)];refs.append(row)
opt=im.u32(60)+24;rv,size=struct.unpack_from('<II',im.b,opt+136);cursor=im.offset(im.base+rv);limit=cursor+size;datarefs=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and cursor+length<=limit
 for off in range(cursor+8,cursor+length,2):
  entry=im.u16(off);assert entry>>12 in (0,3)
  if entry>>12==3:
   address=im.base+page+(entry&4095);value=im.word(address)
   if start<=value<end:datarefs.append(dict(address=hex(address),target=hex(value)))
 cursor+=length
assert cursor==limit
report=dict(sha256=sha,state_start=hex(start),state_end=hex(end),verified_disassembly_references=refs,pe_relocation_references=datarefs,finding='Exact state-range references found only in initializer. No draw routine identified by immediate operands/relocations in this DLL.',limits='Not an alias analysis or proof of absent consumers. Computed pointers, exported data, other modules and runtime writes may consume state. Do not implement an invented stepping algorithm or assume thread safety.',next_action='Trace full module/global ownership while completing actual CPU capability binding for math startup.')
Path('outputs/sfm64-seed-consumer-reference-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print('seed_state_instruction_references='+str(len(refs))+' relocation_references='+str(len(datarefs))+' consumer_identified=false')
