from pathlib import Path
import json,struct
exec(Path('work/prepare_command_buffer_creation_native.py').read_text().split('allowed_calls=')[0])
r=json.loads(Path('outputs/sfm64-vertex-tracker-startup-factory.json').read_text());p=json.loads(Path('outputs/sfm64-vertex-allocation-tracker-provider.json').read_text())
assert sha==r['sha256']==p['sha256']
bodies=[r['wrapper_factory'],r['local_factory_getter'],r['local_lookup'],p['factory'],*p['slot_bodies']]
header='#pragma once\n';metadata=[]
for i,body in enumerate(bodies):
 begin=int(body['start'],16);end=int(body['end'],16);raw=im.b[im.offset(begin):im.offset(begin)+end-begin]
 assert raw==b''.join(bytes.fromhex(x['bytes']) for x in body['instructions'])
 fixups=[(a-begin,im.word(a)) for a in relocations if begin<=a<end]
 assert all(v in {0x10183b48,0x100d5fe0,0x1019fbd8,0x1016d048} for o,v in fixups)
 calls=[(int(x['address'],16)-begin,int(x['target'],16)) for x in body['direct_calls']]
 assert all(t==0x100d6060 for o,t in calls)
 header+='static const unsigned char trackerFactoryBytes'+str(i)+'[]={'+','.join(map(str,raw))+'};\n'
 for name,entries in [('Fixups',fixups),('Calls',calls)]:header+='static const unsigned trackerFactory'+name+str(i)+'[][2]={'+','.join('{'+str(o)+','+str(v)+'}' for o,v in (entries or [(0,0)]))+'};\n'
 metadata.append(dict(file_offset=im.offset(begin),size=len(raw),fixups=fixups,calls=calls))
header+='struct TrackerFactoryBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*fixups)[2];unsigned fixupCount;const unsigned (*calls)[2];unsigned callCount;};\n'
header+='static const TrackerFactoryBody trackerFactoryBodies[]={'+','.join('{trackerFactoryBytes'+str(i)+','+str(m['size'])+','+str(m['file_offset'])+',trackerFactoryFixups'+str(i)+','+str(len(m['fixups']))+',trackerFactoryCalls'+str(i)+','+str(len(m['calls']))+'}' for i,m in enumerate(metadata))+'};\n'
Path('work/vertex_tracker_factory_native_init.h').write_text(header)
Path('outputs/sfm64-vertex-tracker-factory-native-preparation.json').write_text(json.dumps(dict(sha256=sha,bodies=metadata,native_executed=False),indent=2)+'\n')
print('verified_tracker_factory_graph_bytes='+str(sum(m['size'] for m in metadata)))
