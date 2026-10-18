from pathlib import Path
from pe_image import Image
import re,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
methods=[('IsOperatorDirty',0x10F01E10,0x10BD76AC,0x1076ACA0,0),('ClearOperatorDirtyFlag',0x10F01E20,0x10BD7650,0x1076AD30,4),('Operate',0x10F01E30,0x10BD7600,0x1076ADC0,8),('GetOperatorName',0x10F01E40,0x10BD75B8,0x1076AE50,12),('SetSortKey',0x10F01E70,0x10BD745C,0x1076B0E0,24),('GetSortKey',0x10F01E80,0x10BD7410,0x1076B1A0,28)]
selected=[];pending=None
def save(item):
 if item is None:return
 address,data,line=item;offset=image.offset(address);assert image.b[offset:offset+len(data)]==data
 selected.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
with Path('work/ifm-disassembly.txt').open() as stream:
 for line in stream:
  stripped=line.lstrip()
  if len(stripped)>10 and stripped[8:10]==': ':
   address=int(stripped[:8],16)
   if not 0x1076ACA0<=address<0x1076B230:
    save(pending);pending=None
    if address>=0x1076B230:break
    continue
  match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
  if match:save(pending);pending=[int(match[1],16),bytes.fromhex(match[2]),line.strip()]
  elif pending:
   tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
   if tail:pending[1]+=bytes.fromhex(tail[1])
save(pending)
bindings=[]
for name,registration,string,wrapper,slot in methods:
 assert image.word(registration)==string and image.word(registration+4)==wrapper
 assert image.string(string)=='IDmeOperator_'+name
 bindings.append(dict(name=name,registration=hex(registration),wrapper=hex(wrapper),virtual_byte_offset=slot))
assert image.b[image.offset(0x1076AD16):image.offset(0x1076AD16)+4]==bytes.fromhex('8b02ffd0')
assert image.b[image.offset(0x1076ADA6):image.offset(0x1076ADA6)+5]==bytes.fromhex('8b4204ffd0')
assert image.b[image.offset(0x1076AE36):image.offset(0x1076AE36)+5]==bytes.fromhex('8b4208ffd0')
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),methods=bindings,verified_complete_instructions=selected,
 conclusion='Selection virtual offsets0/4 are IsOperatorDirty/ClearOperatorDirtyFlag; framework ordered execution offset8 is Operate. These names come from installed scripting registrations and wrapper dispatch, not assumed SDK ABI.',
 limitations='Static method naming/dispatch evidence. Per-class dirty implementation, full propagation and native live scheduling remain unverified.')
Path('outputs/sfm64-ifm-operator-method-names.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified operator method wrappers:',len(selected),'instructions; dirty/clear/operate offsets=0/4/8')
