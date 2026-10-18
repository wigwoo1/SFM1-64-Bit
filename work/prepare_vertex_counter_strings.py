from pathlib import Path
import json
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
bodies=[region(0x100d7470,0x100d748c),region(0x100d7640,0x100d769d),region(0x100e5bb0,0x100e5ce5)]
assert bodies[0]['direct_calls']==bodies[2]['direct_calls']==[]
assert [x['target'] for x in bodies[1]['direct_calls']]==['0x100e5bb0']
header=['#pragma once']
for i,b in enumerate(bodies):
 raw=im.b[im.offset(int(b['start'],16)):im.offset(int(b['start'],16))+b['bytes']]
 header.append('static const unsigned char vertexStrings'+str(i)+'[]={'+','.join(hex(x) for x in raw)+'};')
call=bodies[1]['direct_calls'][0]
header.append('static const unsigned vertexStringCallOffset='+str(int(call['address'],16)-int(bodies[1]['start'],16))+';')
Path('work/vertex_counter_strings_native_init.h').write_text('\n'.join(header)+'\n')
Path('outputs/sfm64-vertex-counter-strings-preparation.json').write_text(json.dumps(dict(sha256=sha,bodies=bodies,limits='Private original string execution only; no vertex/GPU execution.'),indent=2)+'\n')
print('verified_vertex_counter_string_bytes='+str(sum(b['bytes'] for b in bodies)))
