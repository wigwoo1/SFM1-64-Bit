"""Inventory relocation requirements; this does not execute native code."""
from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x1001b890,0x1001bede
raw=im.b[im.offset(start):im.offset(start)+end-start]
recovery=json.loads(Path('outputs/sfm64-shader-pass-draw-recovery.json').read_text())
assert recovery['sha256']==hashlib.sha256(im.b).hexdigest()
assert any(r['start']==hex(start) and r['end']==hex(end) and r['bytes']==len(raw) for r in recovery['regions'])
optional=im.u32(60)+24
rva,size=struct.unpack_from('<II',im.b,optional+96+40)
cursor=im.offset(im.base+rva);limit=cursor+size;fixups=[]
while cursor<limit:
 page,length=struct.unpack_from('<II',im.b,cursor)
 if not length:break
 assert length>=8 and cursor+length<=limit
 for offset in range(cursor+8,cursor+length,2):
  entry=im.u16(offset);address=im.base+page+(entry&4095)
  if entry>>12==3 and start<=address<end:
   assert address+4<=end
   fixups.append({'offset':address-start,'target':im.word(address)})
 cursor+=length
calls=[];indirect=[]
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):.*?\bcall\s+(.+)',line)
 if not m:continue
 address=int(m[1],16)
 if not start<=address<end:continue
 operand=m[2].strip()
 if re.fullmatch('[0-9A-F]{8}',operand):
  offset=address-start;target=int(operand,16)
  assert raw[offset]==0xe8 and address+5+struct.unpack_from('<i',raw,offset+1)[0]==target
  calls.append({'offset':offset,'target':target})
 else:indirect.append({'address':hex(address),'operand':operand})
assert {f['target'] for f in fixups}=={0x1017c370,0x100fa5f8,0x100fc7dc,0x100fc764,0x1016d130,0x1019092c,0x1001bee0}
assert {c['target'] for c in calls}=={0x100df5ad,0x10002da0,0x100144a0,0x100167b0,0x1000fc90,0x1000e330}
header='#pragma once\nstatic constexpr unsigned char nativeShaderTransitionBytes[]={'+','.join(map(str,raw))+'};\n'
header+='static constexpr unsigned nativeShaderTransitionFileOffset='+str(im.offset(start))+';\n'
for name,entries in [('Fixups',fixups),('Calls',calls)]:
 header+='static constexpr unsigned nativeShaderTransition'+name+'[][2]={'+','.join('{'+str(e['offset'])+','+str(e['target'])+'}' for e in entries)+'};\n'
targets=[im.word(0x1001bee0+i*4) for i in range(6)]
assert targets==[0x1001bdba,0x1001bdb4,0x1001bdba,0x1001bdd2,0x1001bdc4,0x1001bdd2]
assert all(start<=target<end for target in targets)
assert im.word(0x1001bef8)==0xcccccccc
header+='static constexpr unsigned nativeShaderTransitionJumpOffsets[]={'+','.join(str(target-start) for target in targets)+'};\n'
Path('work/shader_transition_native_init.h').write_text(header)
report={'installed_sha256':recovery['sha256'],'body_bytes':len(raw),'absolute_fixups':fixups,'direct_calls':calls,'indirect_calls':indirect,
 'required_bindings':['security cookie and check','cdecl memcpy','controlled queue flush or prove direct-only branch','fake capability/global/secondary/device vtables with exact x86 calling conventions','private jump table pointing into relocated body','controlled or relocated auxiliary/pixel/conversion/renderstate helpers'],
 'limitations':'Preparation only; no execution. All listed references require remapping before invocation. Full owner storage and writable cache capacity must be provided.'}
report['jump_table']={'original_address':hex(0x1001bee0),'target_offsets':[target-start for target in targets],'entries':6,'following_word':hex(im.word(0x1001bef8)),'domain':'Prototype inputs0..6 only; mode0 takes separate branch. Input7 final dispatch remains unverified.'}
Path('outputs/sfm64-shader-transition-native-preparation.json').write_text(json.dumps(report,indent=2))
print('shader_transition_bytes='+str(len(raw))+' absolute_fixups='+str(len(fixups))+' direct_calls='+str(len(calls))+' indirect_calls='+str(len(indirect)))


