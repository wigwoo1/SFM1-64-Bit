from pathlib import Path
import json,struct
exec(Path('work/prepare_command_buffer_creation_native.py').read_text().split('allowed_calls=')[0])
report=json.loads(Path('outputs/sfm64-vertex-tracker-connector-recovery.json').read_text());assert sha==report['sha256']
body=report['connector'];begin=int(body['start'],16);limit=int(body['end'],16)
raw=im.b[im.offset(begin):im.offset(begin)+limit-begin]
assert raw==b''.join(bytes.fromhex(x['bytes']) for x in body['instructions'])
fixups=[(a-begin,im.word(a)) for a in relocations if begin<=a<limit]
assert {v for o,v in fixups}=={0x101909c8,0x101909c4,0x1017b15c,0x1017b158,0x10190718,0x101523c0,0x100fa23c}
boundaries={int(x['address'],16) for x in body['instructions']}
for row in body['instructions']:
 a=int(row['address'],16);b=bytes.fromhex(row['bytes'])
 if b[0]==0xeb or 0x70<=b[0]<=0x7f:assert a+2+struct.unpack_from('<b',b,1)[0] in boundaries
 elif b[0]==0x0f and 0x80<=b[1]<=0x8f:assert a+6+struct.unpack_from('<i',b,2)[0] in boundaries
assert body['direct_calls']==[]
header='#pragma once\nstatic const unsigned char trackerConnectorBytes[]={'+','.join(map(str,raw))+'};\n'
header+='static const unsigned trackerConnectorFixups[][2]={'+','.join('{'+str(o)+','+str(v)+'}' for o,v in fixups)+'};\n'
header+='static const unsigned trackerConnectorFileOffset='+str(im.offset(begin))+';\n'
Path('work/vertex_tracker_connector_native_init.h').write_text(header)
Path('outputs/sfm64-vertex-tracker-connector-native-preparation.json').write_text(json.dumps(dict(sha256=sha,bytes=len(raw),fixups=fixups,interface_entries=56,contract='Remap allabsoluteoperands and supply cdecl factories(name,nullstatus). Mutable outputcells/bookkeeping isolated. Negativebookkeeping diagnostic callback separate domain.',native_executed=False),indent=2)+'\n')
print('verified_tracker_connector_bytes='+str(len(raw))+' fixups='+str(len(fixups)))
