from pathlib import Path
import json,hashlib
from pe_image import Image
r=json.loads(Path('outputs/sfm64-transition-composed-native-preparation.json').read_text())
blob=Path('outputs/transition-composed-original.bin').read_bytes();assert hashlib.sha256(blob).hexdigest()==r['blob_sha256']
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll');assert hashlib.sha256(im.b).hexdigest()==r['sha256']
globals={0x10183fc4:0,0x1019092c:1,0x1016dd08:2,0x1016e13c:3,0x1016e170:4}
calls=[];fixups=[];bodies=[]
for h in r['bodies']:
 p=h['placement'];n=h['bytes'];o=im.offset(int(h['begin'],16));assert blob[p:p+n]==im.b[o:o+n];bodies.append((p,n,o))
 for c in h['calls']:calls.append((p+c['offset'],c.get('target_placement',0),0 if c['binding']=='internal' else 1 if c['binding']=='getter' else 2))
 for f in h['fixups']:fixups.append((p+f['offset'],globals[int(f['target'],16)]))
def array(name,rows):return 'static const unsigned '+name+'[]['+str(len(rows[0]))+']={'+','.join('{'+','.join(map(str,row))+'}' for row in rows)+'};\n'
header='#pragma once\n'+f'static const unsigned transitionComposedSize={len(blob)},transitionComposedEntry={r["entry_placement"]};\n'
header+=array('transitionComposedBodies',bodies)+array('transitionComposedCalls',calls)+array('transitionComposedFixups',fixups)
for name,rows in r['tables'].items():header+='static const unsigned transitionComposed'+name.title()+'[]={'+','.join(str(row['placement']) for row in rows)+'};\n'
Path('work/transition_composed_native_init.h').write_text(header)
print('transition_composed_runtime_header=verified')
