from pathlib import Path
import hashlib,json
from pe_image import Image
r=json.loads(Path('outputs/sfm64-restore-resource-creation-native-preparation.json').read_text())
blob=Path('outputs/restore-resource-creation-composed-original.bin').read_bytes()
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
assert hashlib.sha256(im.b).hexdigest()==r['sha256'] and hashlib.sha256(blob).hexdigest()==r['blob_sha256']
for b in r['bodies']:assert blob[b['blob_offset']:b['blob_offset']+b['size']]==im.b[b['file_offset']:b['file_offset']+b['size']]
lines=['struct ResourceBody {unsigned file,offset,size;};','static const ResourceBody resourceBodies[]={'+','.join('{%d,%d,%d}'%(b['file_offset'],b['blob_offset'],b['size']) for b in r['bodies'])+'};',
 'struct ResourceCall {unsigned operand,next,target;};','static const ResourceCall resourceCalls[]={'+','.join('{%d,%d,%d}'%(c['operand'],c['next'],c.get('blob_target',4096)) for c in r['calls'])+'};',
 'struct ResourceFixup {unsigned operand,original,binding;};']
bindings=list(dict.fromkeys(f['binding'] for f in r['fixups']))
lines+=['static const ResourceFixup resourceFixups[]={'+','.join('{%d,%s,%d}'%(f['operand'],f['original'],bindings.index(f['binding'])) for f in r['fixups'])+'};',f'static const unsigned resourceBindingCount={len(bindings)};', 'static const char* resourceBindingNames[]={'+','.join('"'+b+'"' for b in bindings)+'};']
Path('work/restore_resource_loader_init.h').write_text('\n'.join(lines)+'\n')
print('resource_loader_header_checked')
