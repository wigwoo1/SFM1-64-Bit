import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
regions=[(0x103597e0,0x1035984a),(0x10359940,0x103599a0),(0x1035d9e0,0x1035daad),(0x10362bd0,0x10362c0c)]
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:continue
    va=int(m[1],16)
    if va>=regions[-1][1]:break
    if not any(a<=va<b for a,b in regions):continue
    raw=bytes.fromhex(m[2]);at=im.offset(va)
    assert im.b[at:at+len(raw)]==raw
    spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(verified_prefixes=spans,findings=[
    '103597e0/10359940 return for null attribute or ffffffff target handle; obtain target reference state using103596d0(handle,null optional saved-record output); missing state no-op. Lookup supports live and saved records.',
    'Add marks record byte8 bit1 then1035d9e0; remove calls1035da30. Manager byteC198 bit10 is set under the recovered orphan-like conditions; meaning pending.',
    '1035d9e0: when head token at record0c valid, allocates8-byte node, copies old head/next, prepends at record10. Obtains attribute token10362bd0(attribute,true), stores as new head. Duplicate tokens are not checked.',
    '1035da30 obtains token with create=true. Matching head promotes first linked node and frees it, or sets head invalid if no node. Otherwise traverses next pointers and unlinks/frees first matching token; absent no-op.',
    '10362bd0 lazily obtains attribute token through1035b4c0 if attribute0c invalid and create true, caches it; failure calls imported diagnostic. Token allocator and diagnostic identity pending.',
    'Add allocation null path still dereferences the result in visible original instructions; no invented graceful failure behavior. Runtime allocation failure handling unresolved.'
],limitations='First-line instruction prefixes checked against installed PE; full continuation bytes, target record lookup/generation checks, token allocation, runtime list behavior and native graph integration remain unresolved.')
Path('outputs/channel-incoming-reference-updates-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified prefixes:',len(spans))
