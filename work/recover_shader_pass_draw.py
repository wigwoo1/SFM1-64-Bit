from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert im.word(0x100fcedc+94*4)==0x10025ea0
assert im.word(0x100fcedc+202*4)==0x10010740
assert im.word(0x100fcedc+93*4)==0x10011000
assert im.word(0x100fbafc+27*4)==0x100063f0
regions=[(0x10025ea0,0x10025f2c),(0x10010740,0x1001074b),(0x10011000,0x1001103f),(0x100063f0,0x100063f1),(0x1002f350,0x1002f407),(0x1002f2e0,0x1002f34b)]
regions += [(0x1002e010,0x1002e06e)] + [(a,a+20) for a in (0x1002ed30,0x1002ed50,0x1002ed70,0x1002ed90,0x1002edb0)]
regions += [(0x1002e070,0x1002e0d4),(0x1002e0e0,0x1002e162)]
regions += [(0x1002eac0,0x1002eafc),(0x1002eb00,0x1002eb25),(0x1002eb30,0x1002eb55),(0x1002eb60,0x1002eba1)]
regions += [(0x1000f580,0x1000f5de),(0x10016500,0x1001652e)]
regions += [(0x1000e330,0x1000e3fc)]
regions += [(0x100f94a0,0x100f94b6),(0x10027240,0x100275f1),(0x1000da70,0x1000da79)]
regions += [(0x100296f0,0x100297bd),(0x100210b0,0x100210da),(0x10021110,0x1002113a)]
regions += [(0x10026890,0x100268f7),(0x1001f820,0x1001f89c),(0x10026ba8,0x10026bb6),(0x10026f87,0x10026f94),(0x10027020,0x10027032)]
regions += [(0x10019910,0x10019a09),(0x10011610,0x10011636)]
regions += [(0x1000f1b0,0x1000f3d0)]
regions += [(0x1001f8a0,0x1001fb8f)]
regions += [(0x10021200,0x100212c0)]
regions += [(0x1001d1f0,0x1001d4b5)]
regions += [(0x1000e290,0x1000e2ee),(0x1000e2f0,0x1000e324)]
regions += [(0x1001b890,0x1001bede)]
regions += [(0x1000fc90,0x1000fe10),(0x100d4ef0,0x100d4f37)]
regions += [(0x100d5020,0x100d5368)]
regions += [(0x100d5520,0x100d5592),(0x10028a0d,0x10028a3e),(0x100d5440,0x100d548c)]
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines()
results=[]
for start,end in regions:
 covered={};address=None;instructions=0
 for line in lines:
  m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
  if m:
   address=int(m[1],16);raw=bytes.fromhex(m[2])
   if start<=address<end:instructions+=1
  elif address is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):raw=bytes.fromhex(line)
  else:continue
  for b in raw:
   if start<=address<end:covered[address]=b
   address+=1
 assert len(covered)==end-start
 raw=bytes(covered[a] for a in range(start,end))
 assert raw==im.b[im.offset(start):im.offset(start)+len(raw)]
 results.append(dict(start=hex(start),end=hex(end),bytes=len(raw),instructions=instructions))
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,vtable='0x100fcedc',slots={'94':'0x10025ea0','202':'0x10010740'},flow=['Primary interface atsecondarythis-720 slot36 lowbyte suppression gates entirepass.','Calls1002f350 atsecondary+11400 withsnapshotwordsecondary+11780, then100254b0 atcompleteobject withreloadedliveword.','Reloadsmeshsecondary+11236: nonnullslot28(instanceword), elsemanager10005bc0 slot21(instanceword). Setslive snapshotword11780 toffff aftercallback. Twoextraarguments unused.','Slot202 writes secondarythis+13120 wordffffffff, nootherstatechange.'],limitations='Staticbody/bindingchecked andlogicalcandidateobserved. Transition/preparehelpers, meshmanagerdraw, cachedwordtype, nativeexecution andGPU unfinished.')
report['slots']['93']='0x10011000'
report['dynamic_mesh_snapshot_notification']={'vtable':'0x100fbafc','slot':27,'target':'0x100063f0','body':'ret; no-op for this specific mesh implementation. Other mesh implementations not inferred.'}
report['fill_transition']={'ordinary_operation':4,'body':'0x1002e010..0x1002e06e','value':'72-byte state record+0xc uint32','queue_packet':[1,8,'value'],'direct':'COM57 SetRenderState(8,value); HRESULT ignored natively'}
report['depth_write_transition']={'ordinary_operation':1,'value':'(state byte+0x44 >>1)&1','queue_packet':[1,14,'value'],'direct':'COM57 SetRenderState(14,value)'}
report['color_write_transition']={'ordinary_operation':2,'value':'state+8 uint32','queue_packet':[1,168,'value'],'direct':'COM57 SetRenderState(168,value)','cache':'After queue write/direct callback, reload live state+8 and ownerglobal10183fc4; write owner+0x140. Native HRESULT ignored.'}
report['cull_interface_target']={'slot':263,'target':'0x1000f580','configured':'secondary+0x1700','cache':'secondary+0x1704','enabled_byte':'secondary+0x1708','flow':'Publish lowbyte input; requested=configured unlesslowbytezero then1. If requested differscache, completeobjectflag+0x2efc bit1 flips non1 modes:2->3, others->2. Call1000e330(22,submitted,0); cache captured unflippedrequested aftercallback.'}
report['slot272_target']={'target':'0x10016500','flow':'Global10180374 object+0x30 nonzero ANDlowbyteargumentnonzero selects selfslot188(), otherwise selfslot189(). Meaning and target implementations unresolved.'}
assert 0x10180460+0x648==0x10180aa8
report['cached_render_state_submission']={'body':'0x1000e330..0x1000e3fc','cache':'completeobject+0x1c98+key*4','flow':'Primaryslot36 lowbyte suppression thencache equality suppresses work. Nonzero thirdarglowbyte calls secondaryslot84. Reload embeddedcommandstate+648 queueflag+10, submit[1,key,value] orCOM57, publish capturedvalue to originalcacheentry aftercallback. HRESULT ignored.','ownership_link':'Static completeobject10180460+648 equals commandgetter10180aa8. Construction/lifetime still unresolved.'}
report['renderer_construction_prefix']={
 'startup':'100f94a0 loads10180460, calls10027240, registers100f9d10 destructor via100df012.',
 'constructor':'Calls base100296f0; constructs secondaryobject at+2d0 via1000da70, temporarily installs100fc21c/100fc678 thenfinal primary100fd33c, secondary100fcedc andaux100fcec0. Calls1000db00 at+2f4.',
 'secondary_base':'1000da70 onlywrites vtable100fbdfc andreturns this.',
 'scope':'Startup andfullconstructor body verified; dependency bodies still unresolved. Command pool construction, device assignment and queue enable remain unresolved.',
 'full_body':'0x10027240..0x100275f1',
 'additional_initialization':'Initializes owned buffer fields+668..690, clears region+1198 sizee78 andotherfixedregions, constructs+2f58 via10031240, initializes intrusive link+33f4 toself, reserves threewords through1002ef20 at+668 andzeros them, then1000df90 finalreset.',
 'command_state_observation':'No direct device/queueflag assignment identified in constructor body; base100296f0/member constructors andlaterdevice setup must be traced. This does not establish queue unreachability.'}
report['renderer_base_constructor']={'body':'0x100296f0..0x100297bd','base_call':'0x100288d0','vtable':'0x100fdc9c','flow':'Initializes baseobject fields through+2cc, clears56bytes at+70 then sets first2words1. No directcommanddevice+648/queueflag+658 writes inthisbody. Base dependency andlaterassignment unresolved.'}
report['renderer_command_state_callbacks']={'regions':['0x100210b0..0x100210da','0x10021110..0x1002113a'],'arguments':['completeobject+0x648 commandstate','completeobject+0x2010 buffer','completeobject+0x1198 buffer',1],'tables':['completeobject+0x2e8c','completeobject+0x2ea0'],'flow':'Callbacks usecdecl fourarguments; livecallbacktable reloaded eachiteration. Targets, registration andowningmethod body unresolved. Passing commandstate doesnot prove assignment or consumption.'}
report['callback_registration']={'body':'0x10026890..0x100268f7','flow':'Select20-byte vector atcomplete+2e8c+category*20, reservegrowth through1002ef20, appendcallback atcapturedoldcount, publish debugstorage pointer+10 andincrementcount+0c. Native category unchecked.', 'observed_targets':[{'callback':'0x1001f8a0','category':1,'site':'0x10026ba8'},{'callback':'0x10019910','category':'liveecx; unresolved here','site':'0x10026f87'},{'callback':'0x10011610','category':0,'site':'0x10027020'}]}
report['callback_drain']={'body':'0x1001f820..0x1001f89c','flow':'Suppression lowbyte gates entiremethod. Capturecategoryvector/count; zero countreturns. Iteratecapturedsignedcount, reloadtableeachiteration, call(commandstate,buffer2010,buffer1198,callerflag); afterward clearcategorydirtybyte andsetvectorcount0. Callback-appended entries would be discardedbycountreset; actualreentrancy contract unresolved.'}
report['viewport_callback']={'body':'0x10019910..0x10019a09','flow':'Forceflag lowbyte bypasses sixrawDWORD equalitycheck. Widthword2/heightword3 zero suppressessubmission/cachepublication. Ignores commandstatearg; usesstatic10180aa8 queue. Queuedopcode20+sixviewportwords (28bytes), else COM47 SetViewport(livestate). After submission copieslive24bytes into cache; HRESULTignored. Native three8-byte copychunks retained; arbitrary aliasing/exception parity unverified.'}
report['callback11610']={'body':'0x10011610..0x10011636','flow':'Usesfirstarg inEAX; forwards current/cache/force andconstants0x400,0x200 to1000f1b0. Dependency unfinished.'}
report['vertex_texture_callback']={
 'body':'0x1001f8a0..0x1001fb8f',
 'iteration':'Capture signed count from global1019092c slot31; nonpositive returns. Stage begins257; current pointer field atbuffer+aa0, cache record atbuffer+aa0; both advance24 bytes per stage.',
 'texture':'Compare retained record pointer unless force lowbyte. Publish cache pointer before submission. Signed32 nonpositive pointer binds null; positive pointer resolves record+48 directly when byte+1a==1, otherwise indexes pointer array at+48 by byte+1b. Queue opcode2,stage,COMpointer or COM65 SetTexture. The pointer packet is native32 only; x64 transport requires a separate pointer-width contract.',
 'samplers':[{'record_offset':0,'cache_offset':4,'type':1},{'record_offset':4,'cache_offset':8,'type':2},{'record_offset':16,'cache_offset':16,'type':6},{'record_offset':12,'cache_offset':12,'type':5},{'record_offset':20,'cache_offset':20,'type':7}],
 'sampler_flow':'Null retained record skips samplers. In listed order read live retained record value, compare cache unless force lowbyte, publish cache before submission, reload queue flag, then opcode17,stage,type,value or COM69 SetSamplerState. HRESULT ignored. Record pointer remains captured across callbacks; fields are reread for each sampler.',
 'limitations':'Static evidence only. Negative nonnull pointers still reach sampler dereferences; valid sentinel domain unresolved. Device lifetime, array capacity, callback aliasing, texture ownership, x64 queue transport and GPU execution remain unverified.'}
report['texture_enable_transition']={'slot':262,'body':'0x10021200..0x100212c0','input':'72-byte state byte48+stage bit0','cache':'secondary+1748+stage*36 byte; texture handle atsecondary+1728+stage*36','flow':'Exact byte equality skips. Enable publishes1 then calls completeobject1001d1f0(stage,capturedhandle,1). Disable publishes0; zero handle skips submission. Nonzero handle queues[2,stage,0] orCOM65 SetTexture(stage,null), ignoresHRESULT. Does not clear stored handle.','command_state':'secondary+378 equals completeobject+648; queueflagsecondary+388','limitations':'1001d1f0 binding helper and handle type/lifetime unresolved; no native runtime verification.'}
report['texture_enable_transition']['pointer_type_evidence']='1001d1f0 dereferences incoming handle as texture record; logical x64 field widened to uintptr_t.'
report['texture_binding_helper']={
 'body':'0x1001d1f0..0x1001d4b5',
 'cache':'completeobject+19f8+stage*36; pointer at0, enabled byte32, sampler fields4..24',
 'gate':'Same record pointer and force lowbytezero returns. Null input or memory budget rejection submits null without changing cached pointer. Budget global10180104+30 signed; if nonnegative and record+34 differs owner+2f14, signed wrapping owner+2e0 + record+2c > budget<<10 rejects.',
 'binding':'Accepted record published before enabled-byte test; disabled stage returns. Resource resolves direct record+48 when byte1a==1 else array indexed by byte1b. Opcode2 or COM65 binding then usage accounting.',
 'accounting':'After binding, reload owner+2f14; differing record+34 updates epoch, resets record+3c, adds record+2c to optional pointed counter atrecord+28 and owner+2e0. Unless owner byte2db nonzero increment record+3c; signed max updates record+38.',
 'samplers':'ADDRESSU1,ADDRESSV2,ADDRESSW3 from record offsets0,4,8, caching before helper1000e2f0. Global10183b58 slot8 configuration byte3c zero forces MINFILTER6 and MAGFILTER5 to0, otherwise record+10/+c. Configuration reloaded again: byte48 zero forces MIPFILTER7 to0, else earlier byte3c zero forces1, otherwise record+14. Each value compared and cached before helper.',
 'limitations':'Static only. Configuration/helper semantics, memory-budget meaning and signed-overflow policy, record ownership and native runtime/GPU parity unresolved. Logical x64 binding helper not implemented.'}
report['sampler_submission_helper']={'wrapper':'0x1000e2f0..0x1000e324','submission':'0x1000e290..0x1000e2ee','flow':'Completeobject primaryslot36 lowbyte suppression gates submission. Otherwise commandstatecomplete+648, value inEDI, stage/type onstack. Reloadqueueflag; packet[17,stage,type,value] orCOM69 SetSamplerState. No cache inhelper andHRESULT ignored. Bindinghelper publishes sampler cache before entering suppressionwrapper, so suppression may leave cache advanced without device change.','limitations':'Native queue ownership and consumer remain unresolved.'}
report['slot264_state_transition']={
 'body':'0x1001b890..0x1001bede',
 'entry':'Calls global1016d130 slot34 before reading arguments/liveowner. Inputmode6 selects constant mode2 branch. Otherwise live secondary+2e40==2 skips parameter uploads; service1019092c slot17 signed<=90 selects mode0, else inputlowbyteflag selects mode1/2.',
 'snapshot_mode':'Inputmode7 resolves current signed16 snapshot secondary+2cdc using72-byte records at+2ce0 and reads record+64; invalidffff selects0. Inputmode remains7 for later dispatch.',
 'constant_uploads':'Two raw16-byte comparisons against cachepointersecondary+1710, offsets256 and32. Changed locals upload vertex register16 or2 count1 via opcode10 orCOM94, then reload two cache pointers(+1710,+2588) and copy local16bytes into both. Local values include ownerfloatfields+2c7c/+2c80/+2c84 and+dec/+df0/+df4/+15d0. Floating branches and native exception behavior need separate recovery.',
 'tail':'Mode0 calls secondaryslot267(0) and optional completeobject100144a0(handle atsecondary+3340,inputmode). Nonzero input dispatches table1001bee0 indexedinputmode-1 to choose color bytes. Stores normalized color floats atsecondary+15b0..15b8. Selectedmode nonzero calls completeobject100167b0; zero compares packed color againstsecondary+15ac, publishes then cached render state34 via1000e330.',
 'limitations':'Static only; jump-table domain/targets, color helper1000fc90, floating arithmetic/NaN policy, flags and completeobject helper meanings unresolved. No logical replacement or GPU parity claimed.'}
color_targets=[im.word(0x1001bee0+4*i) for i in range(6)]
assert color_targets==[0x1001bdba,0x1001bdb4,0x1001bdba,0x1001bdd2,0x1001bdc4,0x1001bdd2]
assert im.word(0x1001bef8)==0xcccccccc
report['slot264_color_dispatch']={'table':'0x1001bee0','verified_targets':[hex(a) for a in color_targets],'modes':{'1_and_3':'RGB128, conversion eligible','2':'RGB0, conversion disabled','4_and_6':'Live ownerbytes+2e3c,+2e3d,+2e3e, conversion eligible','5':'RGB255, conversion disabled'},'conversion':'Eligible AND fourthargument lowbytezero calls1000fc90; otherwise packedffRRGGBB. Stores disablebyte15c0 asNOT(eligible AND fourthargument lowbytezero). Normalized float constant bits3b808081.','domain_gap':'Only six table entries; followingwordcccccccc. Input7 is handled in earlier snapshot resolution but final dispatch still reloads originalinput; safe call domain and reachability need caller/generator evidence. Do not infer seventh entry.'}
report['color_conversion_helper']={'body':'0x1000fc90..0x1000fe10','flow':'Conversion enabled if global1016d130 byte5aa bit0, or fourthargument lowbyte AND(byte5a9 bit7 OR service slot34==2). Global pointer reloaded after conditional callback. Always calls service slot34 again; mode1 selects scaling. Normalize RGB by floatbits3b808081. Enabled conversion calls100d4ef0 for each channel then mode1 scales by completeowner+3114; disabled mode1 scales by+3120. COMISS upperclamp each channel to1 (unordered also selects1), multiply255, CVTTSS2SI and pack lowbyte RGB withalpha255. No lowerclamp here.', 'lookup_helper':'100d4ef0..100d4f37: negative input returns0; compare threshold10152408, at/above returns1; otherwise multiply255, x87FISTP index respecting controlword, return floattable101941d0[index].','lookup_table':'Runtime table contents/initialization unresolved; do not substitute generic gamma/pow function.','limitations':'Native controlword/FPexception parity, table generation/ownership, service callbacks and scaling fields unresolved; no x64 conversion replacement yet.'}
import struct
def installed_double(address):return struct.unpack('<d',im.b[im.offset(address):im.offset(address)+8])[0]
report['color_lookup_initialization_loop']={'body':'0x100d51c0..0x100d5234','iterations':256,'input':'Index converted tofloat32 andmultiplied byfloat32 bits3b808081, thenroundedvalue widened todouble for100e01aa. Result roundedback tofloat32 andstored at101941d0+index*4. Loop also writes twoother tables.','exponent_bits':hex(struct.unpack('<Q',im.b[im.offset(0x100fc898):im.offset(0x100fc898)+8])[0]),'exponent_value':installed_double(0x100fc898),'other_exponent_value':installed_double(0x10152400),'lookup_threshold_bits':hex(im.word(0x10152408)),'limitations':'Mathhelper100e01aa has substantial polynomial/table/SSE implementation; power semantics inferred from argument/constants, notyetnativeoracleverified. Fullinitializer/caller andmathspecialcases unresolved. Generic double literal2.2 differs frominstalled2.200000047683716.'}
report['color_table_initialization_caller']={'wrapper':'0x100d5520..0x100d5592','guard':'Globalbyte10198ae0; nonzero returns. Otherwise imported capability check/warning then sets guard1 before100d5440 and100d5020. Calls100d5eb0(31415926) afterward. Not a thread-safe once primitive by itself; concurrent call contract unresolved.','observed_callsite':'0x10028a0d..0x10028a3e','first_four_arguments':['floatbits '+hex(im.word(0x100fdb70)),'samefloatbits',0,2],'extra_stack_arguments':[1,1,1,1],'auxiliary_table':'100d5440 generates256floatentries at101986e0 via100e529b usingindex andthreeinstalleddoubleconstants; mathhelper semantics unfinished.','limitations':'Caller context/interface initialization, import bindings, native math and multithreading ownership unresolved. Guard set beforegeneration means recursive/reentrant calls skip incomplete tables.'}
report['vertex_float_constant_batching']={'body':'0x1000f1b0..0x1000f3d0','source':'current/cache pointers atstate+0x848; registers16bytes each','flow':'Iterates signed count; lowbyteforce marksallchanged. Four scalar UCOMISS comparisons markunordered/different values dirty; signed zeros compareequal. Consecutivechanged registers accumulate a run. Anunchangedregister flushes pendingrun; finalpendingrun flushed atloopend.','submission':'Opcode10,start,count,4*count rawfloatwords, or COM94 SetVertexShaderConstantF. Aftercallback reloadbothstate+848 pointers andcopyliverun into cache. No native HRESULTcheck.','wrapper_range':'10011610 suppliesstart1024,count512. Actualsupportedregisterdomain/ownerallocation remainunverified.','prototype_difference':'Existing retained shader constant plan caches rawbits and is not established equivalent to this numeric dirty-run policy. No replacement applied without integration evidence.'}
report['interface_transition_handlers']={
 '10':{'body':'0x1002eac0','flow':'Capture signed count fromglobal1019092c slot1. For eachindex0..count-1, reloadglobal1016dd08 andcallslot262(state,index). Count andstateptr retained acrosscallbacks.'},
 '8':{'body':'0x1002eb00','flow':'Readstate byte68bit4 then global1016dd08slot263(flag).'},
 '12':{'body':'0x1002eb30','flow':'Readstate byte69bit3 then global1016dd08slot272(flag).'},
 '9_and_11':{'body':'0x1002eb60','flow':'Global1016dd08slot264(stateword64,byte68bit0,byte68bit7,byte69bit2). Values captured beforecall.'},
 'limitations':'Interface meanings and target bodies unverified; do not infer D3D render-state semantics from flags alone.'}
report['forwarding_handlers']={'owner_global':'0x10183fc4','mapping':{'0':'0x1002ed50 ->0x1002e510','3':'0x1002ed70 ->0x1002e580','5':'0x1002ed30 ->0x1002e1f0','6':'0x1002ed90 ->0x1002e700','7':'0x1002edb0 ->0x1002e8e0'},'arguments':'72-byte state pointer only; second dispatcherargument unused by wrappers. Owner/helper semantics unfinished.'}
report['transition_snapshot_flow']=['Snapshot handle interpreted signed16, indexed into32-byte records atthis+cc. Record+18word selects state kind. Cached handlethis+56 andkindthis+54 gate transition work.','Different kind computes retained transition descriptor pointer usingthis+88 targetkind*20 table, thenoldkind*4. Suppression callback lowbyte gates descriptor application; descriptor low24bits offset andhigh8bits count reread aftercallback.','Calls1002f2e0(targetkind,offset,count), then1002ebb0, publisheskind only if notsuppressed. Handle stillpublished after suppression path.','Alwaysreloads record table aftertransition, calls global1016e20c slots9/10 withrecord words0/1; retains reloadedtablepointer across firstcallback and readssecondword afterward.']
report['transition_byte_dispatch']={
 'body':'0x1002f2e0..0x1002f34b','state_stride':72,'state_table_offset':'0x58','operation_bytes_offset':'0xb8',
 'ordinary_table':'0x1016e13c','ordinary_targets':[hex(im.word(0x1016e13c+4*i)) for i in range(13)],
 'packed_table':'0x1016e170','packed_targets':[hex(im.word(0x1016e170+4*i)) for i in range(2)],
 'following_words':[hex(im.word(0x1016e178+4*i)) for i in range(6)],
 'decode':'opcode bit7 clear: index=opcode&127, arg0. Bit7 set: index=(opcode>>4)&7, arg=opcode&15. Call handler(stateTable+targetkind*72,arg), cdecl cleanup8. Bytepointer/statepointer retained across callbacks.',
 'limits':'No native bounds checks. Only first2 packed entries are code addresses; following6 words are0x7f7fffff float constants, not handlers. Masks permit more indices than table supports; valid opcode domain requires generator recovery. Ordinary table length13 inferred from next table address. Handler implementations and1002ebb0 unresolved.'}
report['flow'].append('Slot93 uses same primary slot36 lowbyte suppression. After callback, captures live meshsecondary+11236, stores low16 handle atsecondary+11780, calls captured mesh27() if nonnull. No transition application occurs in this body; mesh callback mutations remain live.')
Path('outputs/sfm64-shader-pass-draw-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_shader_pass_draw_bytes='+str(sum(r['bytes'] for r in results)))

