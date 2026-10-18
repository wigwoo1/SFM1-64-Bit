import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=0x102b6607:break
    if va<0x102b6180:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x102b6180
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x102b6607
strings=[]
for span in spans:
 for ref in re.findall(r'\b(10[0-9A-F]{6})h',span['instruction']):
  try:
   value=im.string(int(ref,16))
   if 3<len(value)<150 and all((32<=ord(c)<127 or c in chr(10)+chr(13)+chr(9)) for c in value):strings.append(dict(va=ref,value=value))
  except Exception:pass
Path('outputs/editor-suppression-caller-recovery.json').write_text(json.dumps(dict(complete_bytes=1159,instructions=spans,referenced_strings=strings,findings=[
 '102b6180 checks this47c!=0 or any field480/484/488/48c/490==2 and returns without entering suppression.',
 'Normal path calls100bf760 on localebp-210 at102b6237, later calls100bf8c0 on same local at102b65ef before epilogue.',
 'Large intervening operation invokes model and editor helpers; semantic editor class/action and complete behavior remain unresolved. Presence of begin/restore does not prove exception-unwind paths.'
],limitations='Complete bytes and normal-path suppression pairing verified; operation naming, helper effects, global binding, exception paths and native UI port pending.'),indent=2)+'\n')
print('Verified editor suppression caller bytes:',1159)
print(strings)

