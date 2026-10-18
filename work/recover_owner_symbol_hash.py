import json,re
from pathlib import Path
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
start,end=0x109827f0,0x1098282b
spans=[];active=None
for line in Path('work/ifm-disassembly.txt').open():
    m=re.match(r'\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)\s*(.*)',line)
    if not m:
        c=re.fullmatch(r'\s{12}((?:[0-9A-F]{2} ?)+)\s*',line.rstrip())
        if c and active is not None:active['bytes']+=bytes.fromhex(c[1]).hex()
        continue
    va=int(m[1],16)
    if va>=end:break
    if va<start:continue
    active=dict(va=hex(va),bytes=bytes.fromhex(m[2]).hex(),instruction=m[3].strip());spans.append(active)
cursor=start
for span in spans:
    va=int(span['va'],16);raw=bytes.fromhex(span['bytes']);assert cursor==va
    assert im.b[im.offset(va):im.offset(va)+len(raw)]==raw
    cursor+=len(raw)
assert cursor==end
table=[im.word(0x10f1e850+i*4) for i in range(256)]
assert sorted(table)==list(range(256))
report=dict(verified_complete_instructions=spans,complete_bytes=end-start,installed_table=table,findings=[
 '109827f0 receives string inECX; initializes two accumulators0, consumes unsigned bytes in pairs with alternating table lookups.',
 'A=table[byte1 XOR B], B=table[byte2 XOR A]; stops atNUL and returns(A<<8)|B. Empty string0; odd last byte updatesA only.',
 'Installed256-dword table10f1e850 is a permutation0..255. Exact installed snapshot emitted; runtime table mutation not yet excluded.',
 'Case-sensitive byte input; adjacent10982830 performs case conversion and is not the owner symbol caller target.'
],limitations='Complete59-byte routine/table snapshot verified. Runtime table initialization/mutation and original execution parity pending. Candidate requires explicitly supplied table; no platform hash substitution.')
Path('outputs/owner-symbol-hash-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
Path('work/owner_symbol_installed_table_candidate.h').write_text('#pragma once\n#include <array>\n#include <cstdint>\n// Installed PE snapshot only; runtime mutability unverified.\ninline constexpr std::array<std::uint32_t,256> ownerSymbolInstalledTableCandidate{'+','.join(map(str,table))+'};\n')
print('Verified complete owner symbol hash bytes:',end-start,'table permutation entries:',len(table))
