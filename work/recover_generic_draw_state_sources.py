"""Cross-DLL byte evidence for generic shader light/bone state acquisition."""
import hashlib,json,re
from pathlib import Path
from pe_image import Image
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin')
images={name:Image(root/(name+'.dll')) for name in ['stdshader_dx9','shaderapidx9']}
spans=[]
def check(name,begin,end):
 lines=Path('work/'+name+'-disassembly.txt').read_text().splitlines();cursor=begin;image=images[name]
 for n,line in enumerate(lines):
  m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if m and begin<=int(m[1],16)<end:
   address=int(m[1],16);data=bytes.fromhex(m[2])
   c=re.fullmatch(r'\s+((?:[0-9A-F]{2}\s*)+)',lines[n+1])
   if c:data+=bytes.fromhex(c[1])
   assert cursor==address;at=image.offset(address);assert image.b[at:at+len(data)]==data
   spans.append({'binary':name,'address':hex(address),'bytes':data.hex()});cursor+=len(data)
 assert cursor==end,(hex(cursor),hex(end))
check('stdshader_dx9',0x10059912,0x1005996b)
check('stdshader_dx9',0x10059b40,0x10059b74)
check('shaderapidx9',0x1000fbd0,0x1000fbd7)
check('shaderapidx9',0x10014130,0x10014137)
check('shaderapidx9',0x10014160,0x10014251)
check('shaderapidx9',0x10014260,0x1001429a)
check('shaderapidx9',0x10025b50,0x10025bab)
check('shaderapidx9',0x10025d6d,0x10025da2)
check('shaderapidx9',0x100093f0,0x10009416)
check('shaderapidx9',0x10009548,0x10009564)
check('shaderapidx9',0x1000ffb0,0x100100a0)
check('shaderapidx9',0x10010bb0,0x10010c0c)
check('shaderapidx9',0x10010c10,0x10010c39)
check('shaderapidx9',0x10010c40,0x10010d67)
check('shaderapidx9',0x10019a10,0x10019a54)
check('shaderapidx9',0x10005bc0,0x10005bc6)
check('shaderapidx9',0x100f92a0,0x100f92b6)
check('shaderapidx9',0x1000d150,0x1000d15e)
check('shaderapidx9',0x10008310,0x1000831b)
check('shaderapidx9',0x10007f50,0x10007fa0)
check('shaderapidx9',0x1001bf00,0x1001bf7a)
check('shaderapidx9',0x100154a0,0x100154d8)
api=images['shaderapidx9']
for slot,address in [(2,0x1000fbd0),(9,0x10014130),(36,0x10014260)]:assert api.word(0x100fcedc+4*slot)==address
assert api.word(0x100fcedc+253*4)==0x10025b50
assert api.word(0x100fcedc+274*4)==0x1000ffb0
for slot,address in [(96,0x10010bb0),(99,0x10010c40),(178,0x10010c10)]:assert api.word(0x100fcedc+slot*4)==address
assert api.word(0x100fcedc+84*4)==0x10019a10
assert api.word(0x100fbce4+5*4)==0x10008310
assert api.word(0x100fcedc+146*4)==0x1001bf00
# Aggregation uses an adjusted base, so these are the same fields that the
# interface draw method publishes and subsequently clears.
assert 0x2eb8-0x2d0==0x2be8 and 0x2ebc-0x2d0==0x2bec
report={
 'binary_sha256':{name:hashlib.sha256(image.b).hexdigest() for name,image in images.items()},
 'interface_vtable':'shaderapidx9 0x100fcedc; known interface subobject offset0x720',
 'caller_light_initialization':'zero word at frame-0x4a4 and two flag bytes at frame-0x4a0',
 'caller_light_query_condition':'argument byte at frame+0x14 !=0 AND (argument byte at frame+0x18 ==0 OR argument byte at frame+0x1c !=0)',
 'light_query':'interface slot36 copies8 bytes from interface-relative0x1594/0x1598 to supplied output; if flag0x159c is zero, first calls10014160 with interface pointer-0x2d0 then sets valid byte1',
 'bone_query':'interface slot9 returns word at interface-relative0x170c, stored frame-0x49c',
 'cache_invalidation':'routine10025b50 clears interface-relative validity byte0x159c at10025ba4 before downstream draw work, and10025d94 during cleanup. Early config flag at returned record byte0x4c skips draw path. Other aliases/dynamic writes have not been exhaustively ruled out.',
 'draw_record_source':{
  'method':'interface vtable slot253 (byte0x3f4) ->10025b50, ret24 bytes / six x86 stack arguments',
  'count':'second stack argument at frame+0x0c stored interface-relative0x2be8; same as aggregation-base0x2eb8 after adjustment0x2d0',
  'pointer':'third stack argument at frame+0x10 stored interface-relative0x2bec; same as aggregation-base0x2ebc after adjustment0x2d0',
  'lifetime':'fields published before downstream work and set0 during cleanup at10025d76/10025d7c',
  'ownership_evidence':'method stores caller pointer directly; no record copy at publication. Pointee ownership and upstream producer not yet identified'},
 'single_record_producer':{
  'caller':'100093f0 creates stack record at frame-0x64 via interface slot274 (byte0x448), then invokes draw slot253 with first arg this, second arg1, third arg address of record',
  'builder':'1000ffb0 zeros0x4c bytes, then sets record byte0x18 to interface pointer+0x13dc',
  'aggregation_data':'first72 bytes of interface state at0x13dc form baseline comparison input; light count is interface-relative0x1430 (=0x13dc+0x54)',
  'second_flag':'record byte0x2c remains0 on this builder path',
  'additional_outputs':'builder returns interface pointers+0x11b8 and+0x11b6 through two output args; remaining record fields describe other state and are not fully interpreted',
  'lifetime':'stack record survives synchronous draw call; pointed payload is interface-retained state, not owned by stack record',
  'limits':'Only this single-record path established. Batched caller and upstream state setters remain unresolved.'},
 'retained_lighting_setters':{
  'light_array':'slot96 ->10010bb0. Signed effective count=min(requested, min(config word at global1016d130 pointed byte0x544,4)). Calls interface slot84 before storing count at0x1430. For positive count copies count*0x58 bytes to0x1434, then masks control byte0x11b6 with0xe9.',
  'clear_lights':'slot178 ->10010c10. If count0x1430 !=0, calls slot84, masks control byte0x11b6 with0xe9, then stores count0.',
  'ambient':'slot99 ->10010c40. Compares96 bytes at0x11b8 bitwise. If changed copies all96 bytes, then packs first3 words of each4-word group (six groups) through x87 FLD/FSTP into72 bytes at0x13dc. Sets control byte0x11b6 bit0.',
  'limits':'Semantic names inferred from observed consumers/data shapes. x87 float store may change signaling NaNs; raw bit-copy is not yet claimed equivalent. Flush slot84 dependencies and legal light-count policy remain unverified.'},
 'flush_dependency':{
  'entry':'interface slot84 ->10019a10',
  'gate':'if global10183b58 is non-null, call its vtable slot27 and skip remaining work when low result byte0; null global proceeds',
  'preservation':'save interface words0x0de4 and0x0de8 before dependent call, restore afterward',
  'object':'helper10005bc0 returns global object address1017f7c8; static initializer100f92a0 calls constructor1000d150 which assigns vtable100fbce4',
  'target':'global object vtable slot5 ->10008310',
  'target_body':'10008310 adjusts this by0x290 and tail-jumps10007f50',
  'pending_dispatch':{
   'gate':'adjusted object pointer field0x18 !=null AND byte0x30==0 AND byte0x32!=0; otherwise return',
   'enter':'set byte0x30=1 before dependent call, suppressing reentry',
   'ordinary':'if byte0x33==0 invoke pointed interface slot12 with signed -1 and0 arguments',
   'alternate':'if byte0x33!=0 invoke pointed interface slot25 with pointer to adjusted-object byte0x1c, signed -1 and0 arguments',
   'finish':'reload pointed interface field0x18, clear byte0x30 and word0x32 (both pending flags), invoke reloaded interface slot22 with0,0',
   'ordering':'dependent draw callback observes guard1 and pending flags still set; final callback observes guard0 and cleared flags'},
  'limits':'Object resides in zero-filled runtime storage and is constructor-initialized. No original initialization is invoked. Global/pointed object semantic names, callback rendering bodies, failure behavior and execution parity remain unverified.'},
 'implicit_constructor_query':'interface slot2 returns word at interface-relative0x2e40',
 'implicit_query_setter':{
  'entry':'interface slot146 ->1001bf00, one signed/raw32-bit argument',
  'unchanged':'if argument equals word0x2e40, return without flush or update',
  'changed':'call slot84 flush then store argument unchanged at0x2e40',
  'snapshot_dependency':'if signed16 snapshot index at0x2cdc !=-1, resolve72-byte record in array0x2ce0; invoke slot264 with record word0x40 and flags: byte0x44 bit0, byte0x44 bit7, byte0x45 bit2',
  'related_gate':'routine100154a0 returns false if hardware query slot17 <92 or this query word==2; otherwise tests interface slot29 with argument10',
  'limits':'Does not clamp query against light count. Snapshot refresh semantics and name of query field remain unresolved; this does not explain the sampled invalid shader combinations.'},
 'caller_light_override':'when frame-0x4b5 flag is nonzero, set first light flag byte1, second flag byte0, light count0 before VS20/VS30 branch',
 'cold_light_aggregation':{
  'base':'query-interface pointer-0x2d0; offsets below relative to this adjusted base',
  'reset':'count at0x1864=0, two bytes0x1868/0x1869=0',
  'records':'signed positive count at0x2eb8, pointer0x2ebc, stride0x4c; null record word at byte0x18 skips record',
  'first_flag':'set when non-null pointed data first0x48 bytes differ from original static block0x10180158',
  'second_flag':'OR of nonzero record word at byte0x2c, for non-null records only',
  'count':'signed maximum of zero and each non-null pointed data word at byte0x54'},
 'verified_instruction_spans':spans,
 'limits':'Byte-checked cross-DLL recovery, not execution or GPU proof. Raw caller argument/local semantics remain unnamed. Original x86 offsets are evidence, not an x64 object layout. Record ownership/population, cache invalidation and full material branch selection remain unfinished.'}
Path('outputs/sfm64-generic-draw-state-sources-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('draw_state_source_instruction_spans='+str(len(spans)))
