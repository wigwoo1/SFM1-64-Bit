from pathlib import Path
from pe_image import Image
import re,struct,json,hashlib
image=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\studiorender.dll')
ranges=[(0x10001040,0x1000108F),(0x10013A50,0x10013BC3),(0x10013D74,0x10013DCE)]
lines=[]
for line in Path('work/studiorender-disassembly.txt').read_text(encoding='utf-8').splitlines():
 match=re.match(r'\s+([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)',line)
 if not match:continue
 address=int(match[1],16)
 if not any(a<=address<b for a,b in ranges):continue
 prefix=bytes.fromhex(match[2]);offset=image.offset(address);assert image.b[offset:offset+len(prefix)]==prefix;lines.append(line)
lower=struct.unpack_from('<d',image.b,image.offset(0x100703E0))[0]
upper=struct.unpack_from('<d',image.b,image.offset(0x100702B0))[0]
assert lower<0<upper
report=dict(binary_sha256=hashlib.sha256(image.b).hexdigest(),verified_instruction_prefixes=len(lines),lower=lower,upper=upper,observations=['Mesh-flex entry is skipped when all four finite ramp values are strictly between the extracted lower/upper double constants. Boundary branches process the entry.','For processed entries, a valid per-mesh vertex index is mapped to a generation-tagged slot; a missing generation creates a 48-byte output slot via 10001040, capped at 65536 slots.','New slot copies baseline position and normal and optional tangent before per-vertex interpolation. No per-vertex zero-weight branch is present between slot allocation and coefficient calculation.','Final 100010F0 traverses allocated slot count, not all baseline geometry vertices.'],limitations='Offline branch interpretation for finite coefficients. NaN behavior, generation lifecycle, LOD bounds, capacity exhaustion, dispatch flags and native execution remain unresolved. Prefix checks exclude continuation bytes.')
Path('outputs/sfm64-software-flex-slot-selection.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
Path('outputs/sfm64-software-flex-slot-selection-disassembly.txt').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print(f'Software flex entry skip interval: ({lower}, {upper}); {len(lines)} prefixes verified')
