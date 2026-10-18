import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start,end=0x10976910,0x10976ab7
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        continuation=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if continuation and active is not None:active['bytes']+=bytes.fromhex(continuation[1]).hex()
        continue
    va=int(m[1],16)
    if va>=end:break
    if va<start:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip())
    spans.append(active)
cursor=start
for span in spans:
    va=int(span['va'],16);raw=bytes.fromhex(span['bytes']);assert va==cursor
    assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==end
errors={hex(va):im.string(va) for va in [0x10dda3e0,0x10dda354,0x10dda3ac]}
report=dict(verified_complete_instructions=spans,complete_routine_bytes=end-start,diagnostic_strings=errors,findings=[
 '10976910 copies absolute source directly. Relative source with absolute nonnull base copies base, then adds separator/source.',
 'Null or relative base uses10a1a0c7(output,capacity); failure calls imported diagnostic10a7e474. Nonnull relative base appended after109760d0 separator helper.',
 'Append counts clamp to capacity-currentLength-1 and skip signed nonpositive counts; base/source append10a19dd0.',
 'Before source append, nonempty output lacking trailing separator requires length+1<capacity; otherwise diagnostic called, followed by separator+NUL write if diagnostic returns.',
 'Final10975f80(output,backslash) false invokes diagnostic; final slash conversion still occurs if diagnostic returns. No lowercase step here.'
],limitations='Complete423-byte routine verified. CWD helper/imported diagnostic runtime contract, append/separator helper internals and relative-resolution original execution parity remain unverified. Do not assume diagnostic returns safely at capacity failure.')
Path('outputs/element-owner-relative-path-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified complete relative resolver bytes:',end-start,'diagnostics:',errors)
