from pathlib import Path
from pe_image import Image
import re,json,hashlib

def extract(image,path,ranges):
 result=[];pending=None
 def save(item):
  if item is None:return
  address,data,line=item
  if not any(begin<=address<end for begin,end in ranges):return
  offset=image.offset(address)
  assert image.b[offset:offset+len(data)]==data
  result.append(dict(address=hex(address),bytes=data.hex(),instruction=line))
 with Path(path).open(encoding='utf-8') as stream:
  for line in stream:
   match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
   if match:
    save(pending);pending=[int(match[1],16),bytes.fromhex(match[2]),line.strip()]
   elif pending:
    tail=re.fullmatch(r'\s+((?:[0-9A-F]{2} ?)+)\s*',line)
    if tail:pending[1]+=bytes.fromhex(tail[1])
  save(pending)
 return result

ifm=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
engine=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
assert ifm.string(0x10B42BEC)=='VENGINETOOL003'
assert engine.string(0x1036683C)=='VENGINETOOL003'
assert engine.word(0x1040C0B0)==0x103666C4
assert engine.word(0x103666C4+0xC0)==0x101E5790
assert engine.b[engine.offset(0x101E6470):engine.offset(0x101E6470)+6]==bytes.fromhex('B8 B0 C0 40 10 C3')
assert engine.b[engine.offset(0x101E5790):engine.offset(0x101E5790)+7]==bytes.fromhex('D9 05 BC F7 3F 10 C3')
report=dict(ifm_sha256=hashlib.sha256(ifm.b).hexdigest(),engine_sha256=hashlib.sha256(engine.b).hexdigest(),interface='VENGINETOOL003',ifm_global='0x10fd4398',engine_factory='0x101e6470',engine_object='0x1040c0b0',file_object_vptr='0x103666c4',slot=48,method='0x101e5790',time_float_global='0x103ff7bc',
 ifm_instructions=extract(ifm,'work/ifm-disassembly.txt',[(0x10989DBD,0x10989DD7)]),
 engine_instructions=extract(engine,'work/engine-disassembly.txt',[(0x10311730,0x10311745),(0x101E6470,0x101E6476),(0x101E5780,0x101E5787),(0x101E5790,0x101E5797),(0x101E57A0,0x101E57A6),(0x101E57B0,0x101E57C4)]),
 limitations='Static registration, factory, file-backed object vptr and method evidence. Runtime object initialization/dispatch and global time update scheduling remain unverified. ClientFrameTime is a semantic inference from neighboring time/tick accessors and public SDK ordering, not a recovered symbol.')
assert len(report['ifm_instructions'])==7
Path('outputs/sfm64-ifm-puppet-time-interface.json').write_text(json.dumps(report,indent=2)+'\n')
print('Verified',len(report['ifm_instructions']),'IFM instructions and',len(report['engine_instructions']),'engine instructions; slot 48 reads 103FF7BC.')
