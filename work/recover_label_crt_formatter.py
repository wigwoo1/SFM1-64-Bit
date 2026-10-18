import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
wrapper=bytes.fromhex('8bff558becff75146a00ff7510ff750cff7508e83cffffff83c4145dc3')
at=im.offset(0x10a14369)
assert im.b[at:at+len(wrapper)]==wrapper
spans=[]
for line in Path('work/ifm-disassembly.txt').open():
 m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
 if not m:continue
 va=int(m[1],16)
 if va>=0x10a14386:break
 if va<0x10a142bd:continue
 raw=bytes.fromhex(m[2]);offset=im.offset(va)
 assert im.b[offset:offset+len(raw)]==raw
 spans.append(dict(va=hex(va),bytes=raw.hex(),instruction=m[3].strip()))
receipt=dict(wrapper_address='0x10a14369',complete_wrapper_bytes=wrapper.hex(),
 verified_prefixes=spans,findings=[
 '10a14369 forwards destination/capacity/format, explicit null locale, varargs pointer to10a142bd.',
 '10a142bd rejects null format and nonzero-capacity null destination via helper10a16f8b value16h then10a14537, returns-1. Zero-capacity null destination proceeds.',
 'Initial stream storage zero32bytes; remaining capacity unsigned clamped to7fffffff, flags42, current/base pointers destination. Calls10a1f5d4(stream,format,locale,varargs).',
 'If destination exists, decrements remaining stream capacity after format. Nonnegative writes zero at current stream pointer; negative invokes10a1f470(0,stream). Returns underlying formatted result unchanged.',
 'Modern snprintf cannot be assumed interchangeable until10a1f5d4 output/truncation and10a1f470 termination behavior verified.'
 ],limitations='Wrapper complete bytes verified; core first-line prefixes only. CRT function names not independently established; formatter/parser, invalid-parameter handling, locale and stream writer runtime parity pending.')
Path('outputs/label-crt-formatter-recovery.json').write_text(json.dumps(receipt,indent=2))
print('Verified complete forwarding wrapper bytes:',len(wrapper),'core prefixes:',len(spans))
