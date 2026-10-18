import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
ranges=[(0x100d3250,0x100d34d9)]
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
result=dict(complete_code_bytes=649,routines=groups,undo_label=im.string(0x10aadb54),findings=['No current selection context100bf410 => no undo scope or allocation.', 'Otherwise starts model slot66 with identical label pointers and mode0; allocates128 records of16 x86 bytes. Snapshot context count84, live handles78; resolve model slot18 and filter element slot4 with symbol10effc4c; pass null on failed filter to100d10c0.', 'Nonempty gathered records pass100c4b90; empty result falls through100d2230/100bf8f0 then100d12e0/100bf950 fallback collection paths.', 'Manual inspection of the saved disassembly listing for100c2710 shows four sentinel bounds, traversal of selected controls and animation channels/keys, coordinate transforms through104bc710, and expansion of a four-value graph-data range. This is consistent with graph fitting/scaling and is not evidence of a user marquee hit rectangle. The listing shows101f3e50 as a shared ret-4 stub.', 'The collector records and optional graph bounds are then passed into100c4b90; records are released backwards, storage is freed, and model slot67 ends scope.'],limitations="This verifier byte-matches the 649-byte collector only. The observations about100c2710 and101f3e50 come from the saved disassembly listing and are not byte-matched by this script. Selection-record semantics and mutation remain incomplete. No rectangle/marquee gesture or key hit-test behavior is established. The historical filename 'graph-rectangle-selection-recovery' is retained for compatibility and is not an assertion that a user selection rectangle was recovered.")
Path('outputs/graph-rectangle-selection-recovery.json').write_text(json.dumps(result,indent=2)+'\n')
print('Verified rectangle selection:',649,'bytes; undo label:',result['undo_label'])
