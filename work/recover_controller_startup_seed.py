import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x10a12d2e,0x10a12d7f),(0x10a18770,0x10a18792)]
groups=[[] for _ in ranges];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16);active=None
    if va>=ranges[-1][1]:break
    for i,(start,end) in enumerate(ranges):
        if start<=va<end:
            active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());groups[i].append(active);break
for (start,end),spans in zip(ranges,groups):
    cursor=start
    for s in spans:
        va=int(s['va'],16);raw=bytes.fromhex(s['bytes'])
        assert va==cursor and im.b[im.offset(va):im.offset(va)+len(raw)]==raw
        cursor+=len(raw)
    assert cursor==end
result=dict(verified_code_bytes=115,routines=groups,findings=[
 '10a12d2e reads FILETIME via provenGetSystemTimeAsFileTime import. Wrapped64 addsfe624e212ac18000 then unsigned divide10million via10a18770 divisor-high-zero fast path.',
 'Quotient signed highword compared7, then unsigned lowword93406fff; exceeding threshold substitutesffffffffffffffff. Optional output pointer stores both words; startup uses lowword only.',
 'Receipt covers complete81-byte seed helper and34-byte division fast path reached for fixed divisor, not complete division routine.'
],limitations='Static proof and native boundary fixture; no original executable oracle or production Windows clock integration.')
Path('outputs/controller-startup-seed-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified startup seed and division fast path:',115,'bytes')
