"""Verify and document completeobject100167b0, called by slot264."""
from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
start,end=0x100167b0,0x10016a78
covered={};address=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
 if m:address=int(m[1],16);raw=bytes.fromhex(m[2])
 elif address is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):raw=bytes.fromhex(line)
 else:continue
 for b in raw:
  if start<=address<end:covered[address]=b
  address+=1
assert len(covered)==end-start
raw=bytes(covered[a] for a in range(start,end))
assert raw==im.b[im.offset(start):im.offset(start)+len(raw)]
report={
 'body':[hex(start),hex(end)],'verified_bytes':len(raw),
 'installed_sha256':hashlib.sha256(im.b).hexdigest(),
 'initialization':'Call secondary owner+2d0 slot28(0), store result to float32; positive x87 comparison yields SSE reciprocal of stored float, otherwise zero. Initialize RGB zero and alpha reciprocal. Call secondary slot2 for branch mode.',
 'mode0':'Skip RGB and scaling; retain zeros and reciprocal.',
 'mode1':'Capture RGB owner+1880/1884/1888. If owner byte2004 nonzero, transform each captured channel via secondary slot203 in RGB order, storing x87 result to float32.',
 'other_modes':'Global1016d130 slot41 nonzero: read each live owner channel and call double helper100e01aa with exponent at100fc898, narrow to float32. Otherwise read each live channel and call secondary slot203; restore owner pointer after loop.',
 'exponent':struct.unpack_from('<d',im.b,im.offset(0x100fc898))[0],
 'scaling':'Except mode0, live owner byte1890 zero calls global slot34; result1 multiplies RGB by live captured float owner+3114. No upper or lower clamp in this helper.',
 'comparison':'Raw four DWORD comparison against owner cache pointer+2864, register29 (byte offset1d0). Unchanged returns after security-cookie check.',
 'submission':'Changed block: queue owner+658 nonzero publishes opcode7,register29,count1,four float words after possible10002da0 flush; otherwise COM slot109 SetPixelShaderConstantF(register29,local,1).',
 'publication':'After submission reload pointer owner+19ec and copy local16bytes at register29; then reload owner+2864 and copy same local16bytes. Return through security-cookie check.',
 'limitations':['Static recovery only; no execution or full transition integration.','x87 extended comparison versus stored float reciprocal, FP environment, helper special cases and callback mutation need native oracle.','Owner lifetimes, cache capacities, queue transport and GPU execution unverified.']}
Path('outputs/sfm64-shader-mode-tail-recovery.json').write_text(json.dumps(report,indent=2))
print('verified_shader_mode_tail_bytes='+str(len(raw)))
