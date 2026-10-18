"""Verify installed instruction bytes for ambient expansion and upload routing."""
import hashlib,json,re
from pathlib import Path
from pe_image import Image
image=Image(Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll'))
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines()
spans=[]
for begin,end in [(0x10012140,0x1001220d),(0x10010d70,0x10010e8c),
                  (0x10021466,0x10021495),(0x10021793,0x100217fb),
                  (0x10025b70,0x10025b9d)]:
 cursor=begin
 for n,line in enumerate(lines):
  match=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if not match or not begin<=int(match[1],16)<end:continue
  address=int(match[1],16);data=bytes.fromhex(match[2])
  extra=re.fullmatch(r'\s+((?:[0-9A-F]{2}\s*)+)',lines[n+1])
  if extra:data+=bytes.fromhex(extra[1])
  assert address==cursor
  offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
  spans.append({'address':hex(address),'bytes':data.hex()});cursor+=len(data)
 assert cursor==end,(hex(cursor),hex(end))
assert image.word(0x100fa5f8)==0x3f800000
report={
 'binary_sha256':hashlib.sha256(image.b).hexdigest(),
 'draw_source':'Fifth stack argument of 10025b50 is retained at interface+0x2bf0; indexed by 0x224-byte stride in 10021430. This is a larger lighting record, not a 96-byte ambient array stride.',
 'payload':'Record at interface+0x2bec uses 0x4c-byte stride; its pointer at byte0x18 supplies packed RGB and signed count at payload+0x54.',
 'expansion':'10012140 takes destination, unused count, payload. Six groups of three RGB words become six groups of four; each fourth word is constant 0x3f800000 (1.0). First component of each group uses x87 FLD/FSTP; other two use MOVSS.',
 'command_path':'10021793 checks non-null payload. If indexed flag bit0 is clear, calls expansion then sets bit0. Calls 10010d70 with register21 and expanded indexed lighting record.',
 'upload':'10010d70 compares all96 bytes against cached vertex constants. Identical returns. Changed: queued command10 with register, count6, copied96 bytes, or device vtable byte0x178 (slot94) with register,count6,pointer. Copies values into two retained caches afterward.',
 'limits':'Machine-byte recovery only. Full command dispatch, index/pointer lifetime, unmasked x87 exceptions, and GPU rendering parity remain unverified. Owned ambient setter already quiets signaling NaNs in all RGB components.',
 'verified_instruction_spans':spans}
Path('outputs/sfm64-generic-ambient-upload-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('ambient_upload_instruction_spans='+str(len(spans)))
