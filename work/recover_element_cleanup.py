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
    if va>=0x1035b4bb:break
    if va<0x1035b220:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=0x1035b220
for s in spans:
    va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
    assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==0x1035b4bb
Path('outputs/element-cleanup-recovery.json').write_text(json.dumps(dict(complete_bytes=667,instructions=spans,findings=[
 'Global10e9ccf8 slot8 must return0 or1; otherwise no cleanup. Sentinel rejected afterward.',
 'Before elementpool resolution, enumerates incoming references through service slots115/116, resolves attributes slot89 and calls103623b0 on each resolved attribute.',
 'Resolved element calls10360b20 and1035eff0(1), removes UUID bucket entry via10354710/swaplast, then obtains record1035dc10.',
 'Immediate pool release flag: mode0 always; mode2 only when recordc==FFFFFFFF and record4 lower30bits zero. Other modes/occupied record enqueue103534e0/10354d00/10357880 pending record.',
 'Selects factory through managerflagc198 bit4 default40 or symbol34 type lookup10357510; calls element vslot11,1035f900, then factoryslot1(handle).',
 'Immediate flag releases22/9 elementpool10356e70 then sets managerc180 bit1000. Pending branch instead calls manager slot111(handle).'
],limitations='Complete bytes verified, not production execution. Record bitfields/pending semantics/ref mutation callbacks/factory ABI/service state contracts unverified; no invented undo model.'),indent=2)+'\n')
print('Verified element cleanup bytes:',667)
