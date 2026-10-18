from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
root=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin');shader=Image(root/'shaderapidx9.dll');tier=Image(root/'tier0.dll')
def imports(im):
 opt=im.u32(60)+24;d=im.base+im.u32(opt+104);result={}
 while im.word(d+12):
  module=im.string(im.base+im.word(d+12));names=im.word(d) or im.word(d+16);cells=im.word(d+16);i=0
  while im.word(im.base+names+4*i):
   value=im.word(im.base+names+4*i);result[im.base+cells+4*i]=dict(module=module,name=im.string(im.base+value+2) if not value&0x80000000 else '#'+str(value&65535));i+=1
  d+=20
 return result
def exports(im):
 opt=im.u32(60)+24;d=im.base+im.u32(opt+96);names=im.base+im.word(d+32);ordinals=im.base+im.word(d+36);functions=im.base+im.word(d+28);result={}
 for i in range(im.word(d+24)):
  name=im.string(im.base+im.word(names+i*4));ordinal=im.u16(im.offset(ordinals+i*2));result[name]=im.base+im.word(functions+ordinal*4)
 return result
si=imports(shader);ti=imports(tier);te=exports(tier)
assert si[0x100fa258]['name']=='?FindOrCreateCounter@CVProfile@@QAEPAHPBDW4CounterGroup_t@@@Z' and si[0x100fa258]['module'].lower()=='tier0.dll'
assert si[0x100fa224]['name']=='g_VProfCurrentProfile'
assert te[si[0x100fa258]['name']]==0x10013e80 and te['g_VProfCurrentProfile']==0x100789a0
assert ti[0x1002f040]['name']=='GetCurrentThreadId'
rows=[];pending=None
for line in Path('work/tier0-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:pending=dict(address=hex(int(m[1],16)),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
def body(begin,end):
 selected=[r for r in rows if begin<=int(r['address'],16)<end];cursor=begin
 for row in selected:
  a=int(row['address'],16);raw=bytes.fromhex(row['bytes']);assert a==cursor and tier.b[tier.offset(a):tier.offset(a)+len(raw)]==raw;cursor+=len(raw)
 assert cursor==end
 return selected
creation=body(0x10013e80,0x10013f7a);reset=body(0x10013f80,0x10013fc4)
report=dict(shader_sha256=hashlib.sha256(shader.b).hexdigest(),tier0_sha256=hashlib.sha256(tier.b).hexdigest(),import_bindings={hex(k):si[k] for k in (0x100fa224,0x100fa258)},export_bindings={'counter_factory':'0x10013e80','profile_object_export':'0x100789a0'},thread_import=ti[0x1002f040],verified_creation=creation,verified_group_reset=reset,layout={'counter_values':'profile+10c8, DWORD array','group_bytes':'profile+14c8','allocated_name_pointers':'profile+15c8, native DWORD pointer array','count':'profile+19c8','thread_id':'profile+19cc','fallback_counter':'1007a380'},policy='Check signed count+1>=256 before lookup; valid count255 therefore returns shared fallback even for an existing name. Wrong thread likewise returns fallback. Otherwise compare existing names, returning corresponding inline counter address. New name allocated/copied including terminator, counter zeroed, lowbyte group stored, name pointer retained, count incremented and inline counter address returned. Allocation failure not checked before copying.',group_reset='For each count entry compare sign-extended group byte with full supplied integer; only equal counters zeroed, names retained.',limits='Full static byte/provenance verification only. Original service execution, allocator/free provenance, profile destruction, global profile selection and multithread synchronization remain unresolved; no production lifetime or thread-safety proof.',next_action='Prepare original counter-service oracle with thread/compare/allocation bindings; trace profile destruction to free allocated names and reproduce fallback/thread constraints in pointer-wide service.')
cleanup=body(0x10014690,0x1001474f)
instance_cleanup=body(0x10015160,0x10015174)
static_cleanup=body(0x1002e590,0x1002e5a4)
allocator=body(0x1001e482,0x1001e48d)
release=body(0x1001e4d9,0x1001e4e4)
assert bytes.fromhex(static_cleanup[0]['bytes'])==bytes.fromhex('b9a0890710')
assert any(row['address']=='0x100146f3' and row['instruction']=='call        1001E4D9' for row in cleanup)
assert any(row['address']=='0x10014713' and '000019C8h' in row['instruction'] for row in cleanup)
report['verified_profile_cleanup']=cleanup
report['verified_cleanup_wrappers']=[instance_cleanup,static_cleanup]
report['verified_name_allocator_wrapper']=allocator
report['verified_name_release_wrapper']=release
report['cleanup_policy']='General cleanup first frees another profile array; then walks current counter names in ascending index, invokes1001e4d9 (tail1001b026), clears each name pointer after call and resets count to0 after loop. Counter values and group bytes are not zeroed by this loop. Additional tree cleanup follows; full CRT allocator/free implementation and profile member cleanup remain unresolved.'
report['profile_export_correction']='Static cleanup passes100789a0 directly inECX. The exported g_VProfCurrentProfile address is profile object storage, not evidence of an additional pointer indirection. Imported data IAT provides its address to callers.'
report['limits']='Complete selected factory/reset/cleanup/wrapper bytes verified statically. No original service execution, CRT allocator correctness, full profile destructor, global runtime selection or multithread synchronization proof.'
report['next_action']='Execute original factory/reset/cleanup with controlled allocate/free/thread services, verify retained name ownership and pointer-wide counter identity; then integrate resource counters.'
Path('outputs/sfm64-profile-counter-service-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_profile_counter_bytes=250 group_reset_bytes=68 cleanup_bytes=191 cleanup_wrapper_bytes=40 allocator_wrapper_bytes=22 import_export_binding=verified')
