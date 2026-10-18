from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert im.word(0x100fbafc+28*4)==0x1000b8b0
assert im.word(0x100fbce4+21*4)==0x1000b040
regions=[(0x1000b8b0,0x1000bc93),(0x1000b040,0x1000b164),(0x10006d20,0x10006d79),(0x100dfcd0,0x100dfded),(0x100dfe10,0x100dfebb),(0x100e8697,0x100e876f),(0x100eb6cd,0x100eb6f8),(0x100f67b0,0x100f67c4)]
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
targets=[im.word(0x10006d7c+4*i) for i in range(6)]
assert targets==[0x10006d33,0x10006d3a,0x10006d73,0x10006d46,0x10006d59,0x10006d63]
report=dict(sha256=hashlib.sha256(im.b).hexdigest(),regions=results,primitive_count_targets=[hex(t) for t in targets],flow=['Mesh28 routesglobal1017f770 alternatepath1000ab10; otherwise validatesmaterial exceptprimitive8/9, sets ShaderAPI275(instanceword,0,0), walks published ranges1017f754/count1017f75c.','Indexed branch usesmeshword4c/type, meshword58/minvertex, publishedbasevertex1017f734, publishedvertexcount1017f758, rangefirst/count. Queuedopcode4 has6argumentwords; directCOMslot82 gets same values.','Primitive0/10 uses nonindexed branch;8/9 distinct specialized path. Direct-device and queued-command branches present; telemetry counters also mutated.','Manager21 routes alternatepath else mapsprimitive and builds opcode4/directCOM82 draw; fullresource preparation/argument interpretation remainsunfinished.','Count helper meshword4c types1..6 dispatch:1 vertexcount;2 signedindexcount/2;3 zero;4 signedindexcount/3;5 indexcount-2 wrapping32;6 signedindexcount/4. Otherszero.'],limitations='Exact static bodies and selected vtable/jump-table targetschecked. Complete branch semantics, command decoder, liveCOMdevice, native execution andGPU parity unfinished.')
report['flow'][1]='Indexed COM82/opcode4 argument order is type(mesh+4c), signed baseVertex(mesh+58), minVertex(global1017f734), vertexCount(global1017f758), firstIndex(range.first), primitiveCount(helper). Earlier prose reversed baseVertex/minVertex; packet builder itself accepts correctly named logical arguments.'
assert im.word(0x100fa5f4)==0x41800000
assert im.word(0x100fa5f8)==0x3f800000
report['specialized_primitive_8_9']={
 'body_start':'0x1000bb0b','body_end':'0x1000bc0f',
 'factor_source':'global1017fe44 object+2c; bounded by scalar COMISS branches with constants1 and16',
 'conversion':'100dfcd0 then100dfe10, result stored commandstate+8; conversion helpers not recovered',
 'stream_frequencies_before': ['COM102(stream0, uint32(signed range.count /4)|0x40000000)','COM102(stream3,0x80000001)','COM102(stream8,0x80000001)'],
 'indexed_draw': {'type':5,'baseVertex':0,'minVertex':0,'vertexCount':'uint32(n+1)*(n+1)','firstIndex':0,'primitiveCount':'uint32(2*n*(n+2)-4)'},
 'stream_frequencies_after':['COM102(stream0,1)','COM102(stream3,1)','COM102(stream8,1)'],
 'notes':'Uses direct COM calls regardless of ordinary queue branch. Ignores range.first in this body. Native arithmetic wraps32bits. Stream/resource preparation and conversion semantics remain unresolved.'}
report['conversion_helper_evidence']={'upward_rounding':'100dfcd0 optimized SSE body truncates positive fractional bits and adds1 when needed; negative branch truncates toward zero. CPU/FP-mode gate can route100e8697, unresolved.', 'integer_conversion':'100dfe10 optimized path stores x87 result then CVTTSD2SI. Feature flag can route100dfe46 fallback, unresolved.', 'scope':'Optimized bodies checked statically; native execution, fallback semantics and FP exception parity unverified. Specialized adapter takes an explicit integer factor.'}
report['conversion_fallback_evidence']={
 'integer_body':'100dfe46..100dfebb duplicates x87 input, performs FISTP int64/FILD and residual-based adjustment using carry/borrow, returns EDX:EAX and pops both x87 values. Public32-bit caller uses EAX. Exceptional/out-of-range semantics not verified.',
 'rounding_body':'100e8697..100e876f saves/changes control through100eb6cd; classifies exceptional values through100ee6a1; finite arithmetic calls100f67b0; restores control or routes exception handlers100eedae/100eee03.',
 'next_unresolved_targets':['0x100eb6cd','0x100f67b0','0x100ee6a1','0x100eedae','0x100eee03'],
 'scope':'Full fallback bodies checked against installed bytes; dependencies, execution and floating-point environment parity remain unfinished.'}
control=im.word(0x1017c838)
assert control==0x1b3f and (control&0xc00)==0x800
report['finite_rounding_control']={
 'constant_address':'0x1017c838','file_value':hex(control),
 'control_helper':'100eb6cd returns sign-extended prior x87 control word and loads (old & ~mask) | (requested & mask); caller mask0xffff.',
 'arithmetic_helper':'100f67b0 loads double argument, FRNDINT under current rounding mode, stores/reloads double and returns x87 result.',
 'finite_inference':'File control0x1b3f has RC bits0x800 (round toward positive infinity), so finite fallback rounds upward. Body restores prior control through100eb6cd on ordinary finite return.',
 'limitations':'Static evidence only. Runtime mutation of the constant, exception branches and actual native execution not verified.'}
Path('outputs/sfm64-dynamic-mesh-device-draw-recovery.json').write_text(json.dumps(report,indent=2))
print('checked_device_draw_bytes='+str(sum(r['bytes'] for r in results)))
