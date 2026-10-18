from pathlib import Path
import json,sys
sys.path.insert(0,'work')
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
old=json.loads(Path('outputs/sfm64-vertex-tracker-connector-recovery.json').read_text())['interface_table']
names=[]
for i in range(56):
 address=0x1017b158+8*i;name=im.string(im.word(address));cell=im.word(address+4);assert old[i]['name']==name and old[i]['output_cell']==hex(cell);names.append(name)
assert names[1]=='VEngineCvar007' and names[7]=='VFileSystem017'
Path('work/studio_startup_interface_names_candidate.h').write_text('#pragma once\n#include <array>\nstatic constexpr std::array<const char*,56> studioStartupInterfaceNamesCandidate={'+','.join(json.dumps(n) for n in names)+'};\n')
print('verified_startup_interface_names=56')
