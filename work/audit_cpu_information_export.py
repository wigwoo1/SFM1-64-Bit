from pathlib import Path
import json, hashlib, re
from pe_image import Image
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin')
shader=Image(root/'shaderapidx9.dll');opt=shader.u32(60)+24
descriptor=shader.base+shader.u32(opt+104);binding=None
while shader.word(descriptor+12):
 module=shader.string(shader.base+shader.word(descriptor+12))
 names=shader.word(descriptor) or shader.word(descriptor+16);cells=shader.word(descriptor+16);i=0
 while shader.word(shader.base+names+4*i):
  entry=shader.word(shader.base+names+4*i)
  if not entry&0x80000000 and shader.string(shader.base+entry+2)=='GetCPUInformation':
   binding={'module':module,'iat':hex(shader.base+cells+4*i)}
  i+=1
 descriptor+=20
assert binding and binding['iat']=='0x100fa1fc'
image=Image(root/binding['module']);opt=image.u32(60)+24
directory=image.base+image.u32(opt+96)
names=image.base+image.word(directory+32);ordinals=image.base+image.word(directory+36);functions=image.base+image.word(directory+28)
target=None
for i in range(image.word(directory+24)):
 if image.string(image.base+image.word(names+4*i))=='GetCPUInformation':
  ordinal=image.u16(image.offset(ordinals+2*i));target=image.base+image.word(functions+4*ordinal)
assert target
ranges=[(target,0x10002dc4),(0x10002ef0,0x10002ef9),(0x10002f00,0x10002f06),(0x10003010,0x1000302d),(0x10002f40,0x10002fba),(0x10003890,0x10003938),(0x10003940,0x1000395c),(0x10002dd0,0x10002eb1)]
ranges.extend([(0x10003190,0x1000329b),(0x100036b0,0x10003746)])
ranges.append((0x100032a0,0x100035d1))
ranges.append((0x100030e0,0x10003162))
ranges.append((0x10002ec0,0x10002eec))
rows=[];pending=None
for line in Path('work/tier0-disassembly.txt').read_text().splitlines():
 match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if match:
  pending=None
  if any(start<=int(match[1],16)<end for start,end in ranges):
   pending={'address':hex(int(match[1],16)),'bytes':bytes.fromhex(match[2]).hex(),'instruction':match[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
assert rows and int(rows[0]['address'],16)==target
covered=set()
for row in rows:
 address=int(row['address'],16);raw=bytes.fromhex(row['bytes']);offset=image.offset(address)
 assert image.b[offset:offset+len(raw)]==raw;covered.update(range(address,address+len(raw)))
assert covered=={address for start,end in ranges for address in range(start,end)}
assert image.word(0x1002f578+12)==0x10003010
assert image.word(0x1002f560+12)==0x10002f00
assert image.word(0x1002f590+12)==0x100036b0
report={'import_binding':binding,'export_address':hex(target),'dll_sha256':hashlib.sha256(image.b).hexdigest(),'verified_bytes':len(covered),'verified_instruction_rows':rows,'cached_record':'0x1003bcc0, size sentinel24','byte6_writes':['copy byte5 at10002aa0','helper10002ef0 AL at10002abd','zero-count fallback word101 at10002ad9'],'limits':'Full initializer bytes verified statically; helper dependencies and byte6 meaning not yet recovered. No DLL execution.'}
report['verified_regions']=[{'start':hex(start),'end':hex(end),'bytes':end-start} for start,end in ranges]
report['topology_dispatch']={'wrapper':'10002ef0 dereferences wrapper object then tail-dispatches vtable slot3','windows_backend_slot3':'10003010 counts relationship DWORD at record+4 equal0 over count records, stride24','fallback_slot3':'10002f00 returns1','constructor':'10003890 chooses Windows topology backend when dynamically resolved service is available; alternative CPUID backend remains unrecovered','byte6_semantics_limit':'Non-AMD branch copies byte5. AMD topology path stores AL of backend count, truncating to8bits. Full backend selection and CPU binding unverified.'}
resolver_strings={hex(address):image.string(address) for address in [0x1002f550,0x1002f53c,0x1002f528,0x1002f508]}
assert resolver_strings['0x1002f508']=='GetLogicalProcessorInformation'
report['topology_resolver']={'strings':resolver_strings,'region':'10002dd0..10002eb1','gating':'VerifyVersionInfoA requires requested version6.0 with greater-or-equal conditions; gating execution not verified','buffer_constructor':'10002f40 first calls resolved function with null buffer and length pointer, allocates resulting length, calls again, computes floor(length/24) using multiplyAAAAAAAB and shift4','failure_limit':'Native constructor does not check either API return or allocation result before publishing pointer/count. Correct x64 failure handling requires explicit adaptation; do not reuse original stride for host records.'}
report['alternative_counter']={'vtable':'1002f590 slot3 =100036b0','normalized_input':'count byte+24; ID bytes+4; mask byte+2c OR byte+2a; shift byte+27','algorithm':'count distinct uint8((ID & mask) >> (shift &31)); byte shifts >=8 yield0','limits':'Original local seen-ID buffer has32 entries. Constructor, affinity operations and native execution of this routine remain unverified.'}
imports={};descriptor=image.base+image.u32(opt+104)
while image.word(descriptor+12):
 module=image.string(image.base+image.word(descriptor+12));names=image.word(descriptor) or image.word(descriptor+16);cells=image.word(descriptor+16);index=0
 while image.word(image.base+names+4*index):
  entry=image.word(image.base+names+4*index)
  if not entry&0x80000000:imports[hex(image.base+cells+4*index)]={'module':module,'name':image.string(image.base+entry+2)}
  index+=1
 descriptor+=20
affinity={key:imports[key] for key in ['0x1002f068','0x1002f06c','0x1002f070','0x1002f074','0x1002f078','0x1002f07c']}
assert affinity['0x1002f070']['name']=='SetThreadAffinityMask'
assert affinity['0x1002f078']['name']=='SetProcessAffinityMask'
report['alternative_constructor']={'region':'100032a0..100035d1','verified_imports':affinity,'flow':'Get current process/thread; query process/system masks. System mask1 appends ID0 and returns. Otherwise temporarily widen process mask to system mask if different; iterate32-bit system-mask bits, set thread affinity to each selected bit, Sleep0, store CPUID leaf1 EBX high byte and increment byte count. Restore process mask and saved initial nonzero thread mask; Sleep0.','failure_limit':'Affinity and mask-query return values are not validated before proceeding; full CPUID mask derivation and lifecycle execution pending. Use controlled service replacements for native constructor execution rather than changing live process affinity.'}
report['constructor_mask_inputs']={'default':'total1, cores1','leaf1':'if basic maximum>=1, read leaf1; EDX bit28 clear retains defaults, set extracts total from EBX bits16..23','intel':'if GenuineIntel and basic maximum>=4, cores=(leaf4 EAX>>26)+1','other_vendor':'if extended maximum>=8 (literal recovered unsigned comparison), leaf80000008 ECX bits12..15 nonzero sets cores=1<<bits, otherwise cores=(ECX lowbyte)+1','publication':'helper100030e0 receives only lowbytes of total/cores; e.g. core256 wraps0 and can cause dividebyzero','limits':'Input formula statically verified within audited constructor; exact CPUID cache/lazy callback order and native execution still pending.'}
report['cpuid_wrapper']={'region':'10002ec0..10002eec','arguments':'EAX=argument1 OR argument2; ECX=0 before CPUID','output':'EAX,EBX,ECX,EDX DWORDs at object+0,+4,+8,+c','important_limit':'Second argument is not CPUID subleaf. Existing constructor calls pair leaf4 with0, and extended prefix80000000 with index8.'}
report['topology_dispatch']['byte6_semantics_limit']='AMD string comparison equal0 copies byte5 into byte6; other vendors obtain backend count and store AL. Prior branch-direction description was reversed.'
assert imports['0x1002f05c']['name']=='GetSystemInfo'
report['cpu_count_publication']={'logical':'GetSystemInfo dwNumberOfProcessors lowbyte at record+5','byte6':'AuthenticAMD copies logical byte; other vendors use backend count lowbyte','zero_fallback':'only when both bytes zero, replace both with1','hyperthread_flag':'bit7 at record+4 set when byte6<byte5','execution_limit':'Count publication branches statically verified only; prior AMD/non-AMD interpretation corrected.'}
Path('outputs/sfm64-cpu-information-export-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print('cpu_information_module='+binding['module']+' export='+hex(target)+' verified_bytes='+str(len(covered)))
