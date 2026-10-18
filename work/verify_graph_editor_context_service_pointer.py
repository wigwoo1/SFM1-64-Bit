import hashlib,json,re,struct
from pathlib import Path
binary=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
b=binary.read_bytes(); pe=struct.unpack_from('<I',b,60)[0]; opt=pe+24; base=struct.unpack_from('<I',b,opt+28)[0]; section_count=struct.unpack_from('<H',b,pe+6)[0]; sh=opt+struct.unpack_from('<H',b,pe+20)[0]; target=0x10F4C7E4; sec=None
for i in range(section_count):
 o=sh+i*40; name=b[o:o+8].split(b'\0')[0].decode('ascii','replace'); virtual_size,va,raw_size,raw_ptr=struct.unpack_from('<IIII',b,o+8)
 if va<=target-base<va+max(virtual_size,raw_size): sec={'name':name,'va':hex(base+va),'virtual_size':hex(virtual_size),'raw_size':hex(raw_size),'target_offset':hex(target-base-va),'zero_fill_tail':target-base-va>=raw_size}
assert sec and sec['name']=='.data' and sec['zero_fill_tail']
def rva_offset(rva):
 for j in range(section_count):
  q=sh+j*40; va=struct.unpack_from('<I',b,q+12)[0]; vs=struct.unpack_from('<I',b,q+8)[0]; rs=struct.unpack_from('<I',b,q+16)[0]; rp=struct.unpack_from('<I',b,q+20)[0]
  if va<=rva<va+max(vs,rs) and rva-va<rs:return rp+rva-va
 raise ValueError('RVA is not file-backed: '+hex(rva))
def cstr(rva):
 q=rva_offset(rva); return b[q:b.index(b'\0',q)].decode('ascii','replace')
import_rva,import_size=struct.unpack_from('<II',b,opt+96+8); desc=rva_offset(import_rva); import_ranges=[]; import_count=0
while any(b[desc:desc+20]):
 oft,_,_,name_rva,ft=struct.unpack_from('<IIIII',b,desc); desc+=20; import_count+=1
 lib=cstr(name_rva); th=oft or ft; ti=rva_offset(th); count=0
 while struct.unpack_from('<I',b,ti+4*count)[0]: count+=1
 import_ranges.append({'library':lib,'first_thunk_rva':ft,'slots':count,'end_rva':ft+4*count})
exp_rva,exp_size=struct.unpack_from('<II',b,opt+96); exports=[]
if exp_rva:
 eo=rva_offset(exp_rva); num_names=struct.unpack_from('<I',b,eo+24)[0]; names_rva=struct.unpack_from('<I',b,eo+32)[0]
 for k in range(num_names): exports.append(cstr(struct.unpack_from('<I',b,rva_offset(names_rva)+4*k)[0]))
target_rva=target-base; iat_covers=any(x['first_thunk_rva']<=target_rva<x['end_rva'] for x in import_ranges)
assert import_count==19 and not iat_covers
assert set(exports)=={'CreateInterface','NvOptimusEnablement','init_datamodel','init_dmeutils','init_movieobjects'}
refs=[]
for line in Path('work/ifm-disassembly.txt').open(errors='replace'):
 if re.search(r'10F4C7E4H',line,re.I): refs.append(line.strip())
# Direct absolute writes are distinct from reads through this global pointer.
writes=[line for line in refs if re.search(r'\b(?:mov|and|or|xor|add|sub|inc|dec|xchg)\s+[^,]*ds:\[10F4C7E4h\]\s*,',line,re.I)]
assert not writes,writes
sha=hashlib.sha256(b).hexdigest(); assert sha=='8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'
r={'binary':str(binary),'sha256':sha,'global_va':hex(target),'section':sec,'disassembly_reference_count':len(refs),'direct_absolute_store_count':len(writes),'import_descriptor_count':import_count,'iat_rva_range':[hex(min(x['first_thunk_rva'] for x in import_ranges)),hex(max(x['end_rva'] for x in import_ranges))],'target_rva':hex(target_rva),'target_is_iat_slot':iat_covers,'named_exports':exports,'sample_references':refs[:8],'finding':'The service-pointer storage is in the zero-filled tail of .data, outside the IAT; the five named exports are CreateInterface, NvOptimusEnablement, init_datamodel, init_dmeutils, and init_movieobjects. The saved disassembly has 173 references and no direct absolute store. The DLL image does not reveal the pointer value or target vtable; runtime/host initialization remains necessary to resolve it. This does not rule out aliases or another module writing it.'}
Path('outputs/graph-editor-context-service-pointer-provenance.json').write_text(json.dumps(r,indent=2)+'\n')
print(json.dumps({'sha256':sha,'section':sec,'references':len(refs),'direct_stores':len(writes)},indent=2))
