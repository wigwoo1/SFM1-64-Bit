from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
ranges=[(0x1002b3c0,0x1002b4c0),(0x1002b300,0x1002b3c0),(0x100283e0,0x100283f1),(0x1002aaa0,0x1002aad1),(0x1002abe0,0x1002ac61),(0x1002b4c0,0x1002b653),(0x10028330,0x100283a6),(0x100d74d0,0x100d754e),(0x1002a9d0,0x1002aa94),(0x1002b240,0x1002b2fc),(0x1002b1e0,0x1002b237),(0x10027240,0x10027275),(0x1001fb90,0x100211e7),(0x1000e550,0x1000ea5f),(0x1000e490,0x1000e53c),(0x10015bd0,0x1001624f),(0x10003990,0x100039a7),(0x100039b0,0x100039c7)];rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if match:
  pending=None;address=int(match[1],16)
  if any(start<=address<end for start,end in ranges):pending={'address':hex(address),'bytes':bytes.fromhex(match[2]).hex(),'instruction':match[3].strip()};rows.append(pending)
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line).hex()
covered=set()
for row in rows:
 address=int(row['address'],16);raw=bytes.fromhex(row['bytes']);offset=image.offset(address);assert image.b[offset:offset+len(raw)]==raw;covered.update(range(address,address+len(raw)))
assert covered=={address for start,end in ranges for address in range(start,end)}
vtable_target=image.word(0x100fdc9c);assert vtable_target==0x1002b3c0
limit=struct.unpack_from('<d',image.b,image.offset(0x100fc8c8))[0]
strings={hex(address):image.string(address) for address in [0x100fe808,0x100fe7b0,0x100fe79c,0x100fe764,0x100fe92c,0x100fe8e8,0x100fe8d4,0x100fe8a0,0x100fe840,0x100fd784,0x100fd774,0x100fda68]}
opt=image.u32(60)+24;descriptor=image.base+image.u32(opt+104);bindings={};wanted={0x100fa0dc,0x100fa20c,0x100fa210,0x100fa244,0x100fa1f4,0x100fa238}
while image.word(descriptor+12):
 module=image.string(image.base+image.word(descriptor+12));names=image.word(descriptor) or image.word(descriptor+16);cells=image.word(descriptor+16);index=0
 while image.word(image.base+names+index*4):
  entry=image.word(image.base+names+index*4);iat=image.base+cells+index*4
  if iat in wanted:
   assert not entry&0x80000000;bindings[hex(iat)]={'module':module,'name':image.string(image.base+entry+2)}
  index+=1
 descriptor+=20
assert len(bindings)==len(wanted)
report={'sha256':hashlib.sha256(image.b).hexdigest(),'verified_bytes':len(covered),'instructions':rows,'owner_vtable_slot0':hex(vtable_target),'import_bindings':bindings,'owner_gate':'100283e0 compares imported no-argument result with owner+20 stored DWORD and returns equality byte.','referenced_strings':strings,'query_poll_time_limit':limit,'owner_flow':'Two gates100283e0 and import100fa20c; gated-off clearsflag20 and service slot37(true). Gated-on incrementsowner+2cc; olddepth nonzero logs and returns. First entry callsservice notification,1002b300 query cleanup,1002aaa0,1002abe0; interface/global callbacks then setsflag20.','resource_flow':'Release owner+c4 nonnull then clear. For two query pointers owner+b4/b8: if pointer nonnull and corresponding active bytebc/bd set, poll COMslot7 GetData(outputDWORD,size4,flags1) while HRESULT1 and elapsed time<=constant; then Release slot2, clear pointer and activebyte. Null pointer leaves activebyte unchanged.','limits':'Static verified only. Import identities and owning-thread gate identified; service interfaces, timer lifecycle/FP semantics, query ownership and other resource helpers remain unbound; no GPU execution.'}
report['additional_resource_helpers']={'1002aaa0':'Clear byte owner+c8 before iteration; walk20 live COM pointers owner+cc..118 ascending; for each nonnull call slot2 Release then clear pointer after callback.', '1002abe0':'Walk5 live COM pointers owner+178..188 ascending; for each nonnull call slot2 Release then clear pointer after callback.', 'limits':'Exact interface/resource kinds and callback mutation effects unverified; no native execution of these helpers yet.'}
report['post_reset']={'region':'1002b4c0..1002b653','flow':'Same thread gates. Failed gate queues7 if first argument true then always2. Second argument true forces depth0, warning only signed depth>1. Otherwise decrement modulo32, continue recreation only result0; nonzero logs Warning/DevWarning then reloads depth, clears negative to0 and returns. Optional owner slot41(true), notification slot185 then unconditional live service slot250; helpers1002b240 and1002a9d0; command service slot3; dispatch slot15(callback10028330,0); optional notification end slot186.', 'limits':'Static recovery only. Resource creation helpers statically identified; callback10028330 is an interface factory, receiving service semantics unresolved; flag20 is not directly cleared by this body.'}
report['post_interface_factory']={'region':'10028330..100283a6','comparison':'100d74d0..100d754e ASCII case-insensitive equality','interfaces':{'ShaderDevice002':'global10183b5c','ShaderApi030':'global10183b4c','ShaderShadow010':'global10183b60'},'return_code':'optional int pointer set0 before lookup, unknown name sets1 and returnsnull; matched name returns live global even ifnull with status0','correction':'callback10028330 is an interface factory; dispatch slot15 receiving it is not proven a deferred scheduling API'}
report['query_recreation']={'queued':'1002a9d0: cache+6c==1 sets active+c8=1 and creates20 type8 queries at+cc through live command state getter/flush/device reload each iteration; exact HRESULT8876086a clears output slot and active, continues all20. Cache!=1 clears active and all20slots without Release.', 'pair':'1002b240: optional diagnostic, helper1002b1e0 then live cache+6c. Zero clears2queries/active; nonzero creates2type8 queries with live getter/flush/device reload; exact8876086a warns, otherwise Issue slot6 flag1 then activebyte1.', 'limits':'Helper1002b1e0 identified as texture recreation; cache invalidation unbound; other HRESULT failure values not specially handled, no original or GPU execution yet.'}
report['resource_c4_recreation']={'region':'1002b1e0..1002b237','flow':'Release prior owner+c4 and clear after callback, command state getter then flush then live device reload. Device slot23 receives width1,height1,levels0,usage200hex,format21decimal,pool0,outputowner+c4,sharedhandlenull. Negative HRESULT clears output after call.', 'limits':'CreateTexture signature/SDK constants compiled and controlled original/x64 comparison passed; output ownership on failed call and actualGPU execution unverified.'}
assert image.word(0x100fd33c)==0x1002b3c0 and image.word(0x100fd33c+41*4)==0x1001fb90
report['derived_owner_restore']={'constructor_prefix':'10027240..10027275 calls base100296f0 then replaces owner vtable with100fd33c','slot0':'same cleanup1002b3c0','slot41':'1001fb90; base table slot41 points100dee9e rather than concrete restore','verified_restore_prefix':'1001fb90..1001fbc8 accesses subobjectowner+2d0 slot183(0), clearsbyte+2e88','limits':'Only prefix verified; full restore routine and derivedowner fieldlayout/resource effects unbound. Base-only owner candidate is insufficient for fullreset.'}
restore_rows=[r for r in rows if 0x1001fb90<=int(r['address'],16)<0x100211e7]
report['derived_owner_restore']['full_verified_region']='1001fb90..100211e7'
report['derived_owner_restore']['full_verified_bytes']=0x100211e7-0x1001fb90
report['derived_owner_restore']['call_inventory']=[r for r in restore_rows if r['instruction'].startswith('call')]
report['derived_owner_restore']['limits']='Full raw instruction bytes verified; semantics/dependencies not fully recovered, no execution or x64 full restore implementation.'
report['restore_render_state_helpers']={'defaults_region':'1000e550..1000ea5f','setter_region':'1000e490..1000e53c','setter_flow':'Owner vtable slot36 AL true skips all. Otherwise queuedflag+658 nonzero: reserve3DWORD via10002da0 if needed, emit opcode1/state/value then cacheowner+1c98[state]=value. Directpathdevice+648 slot57 receives state/value, then same cachewrite regardlessHRESULT.', 'limits':'Default setter list/other dependencies not fully mapped, stateindex bounds/nativecallback mutation/realGPU execution unverified.'}
report['constant_storage_restore']={'region':'10015bd0..1001624f','initial_flow':'For existingowner+19ec and+2864 pointers call100039b0, reload globalcapabilities1016d130+528, multiplycount16 with overflowmappedffffffff, allocate10003990, publishpointer. Samepairedpatternforotherconstanttypes,fullsemanticmappingpending.', 'limits':'Allocationfailure/ownership/completebuffer mapping unverified. No originalexecution or x64fullhelperimplementation.'}
report['constant_allocator']={'allocation_wrapper':'10003990..100039a7 dereferences imported allocator global100fa238 then slot1(bytes)','free_wrapper':'100039b0..100039c7 same live allocator global slot5(pointer)','limits':'Static exact wrapper/import evidence only, no originalallocator invocation or x64allocatorABIimplementation.'}
Path('outputs/sfm64-owner-reset-cleanup-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('owner_cleanup_verified_bytes='+str(len(covered))+' query_poll_limit='+str(limit))

