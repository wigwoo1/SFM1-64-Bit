from pathlib import Path
from collections import Counter
import hashlib,json
from pe_image import Image

image=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
prepared=json.loads(Path('outputs/sfm64-constant-storage-restore-native-preparation.json').read_text())
recovered=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text())
allocations=json.loads(Path('outputs/sfm64-constant-storage-allocation-map.json').read_text())
assert hashlib.sha256(image.b).hexdigest()==prepared['dll_sha256']==recovered['sha256']==allocations['source_sha256']
start,end=0x10015bd0,0x1001624f
covered={}
for row in recovered['instructions']:
 address=int(row['address'],16)
 if start<=address<end:
  for i,value in enumerate(bytes.fromhex(row['bytes'])):covered[address+i]=value
assert image.b[image.offset(start):image.offset(start)+end-start]==bytes(covered[a] for a in range(start,end))
direct_counts=Counter(c['target'] for c in prepared['calls'])
assert direct_counts=={0x10003990:12,0x100039b0:12,0x100df030:6,0x10002da0:2}
global_counts=Counter(f['target'] for f in prepared['fixups'])
assert global_counts=={0x1016d130:20,0x100fa5fc:1,0x1016e20c:1}
indirect={int(c['address'],16):c['instruction'] for c in prepared['indirect_calls']}
expected={0x10015e2a:('device',109),0x10015e9b:('owner+2d0',43),0x10015eed:('owner+2d0',44),0x1001613d:('device',94),0x100161bb:('owner+2d0',41),0x10016217:('owner+2d0',42),0x10016239:('owner+2d0',154),0x10016246:('shader manager',11)}
assert set(indirect)==set(expected)
counts=[4,2,2,256,2,2]
buffers=[]
for id in range(6):
 pair=allocations['allocations_in_order'][id*2:id*2+2]
 assert len(pair)==2 and pair[0]['capability_count_offset']==pair[1]['capability_count_offset']
 assert pair[0]['element_bytes']==pair[1]['element_bytes']
 buffers.append(dict(id=id,count_offset=pair[0]['capability_count_offset'],fixture_count=counts[id],bytes_per_allocation=counts[id]*pair[0]['element_bytes'],owner_primary=pair[0]['owner_pointer_offset'],owner_staging=pair[1]['owner_pointer_offset']))
assert counts[3]>=226
report=dict(dll_sha256=prepared['dll_sha256'],verified_body_bytes=end-start,external_direct_calls={hex(t):n for t,n in direct_counts.items()},external_globals={hex(t):n for t,n in global_counts.items()},indirect_bindings=[dict(call=hex(a),receiver=receiver,slot=slot,instruction=indirect[a]) for a,(receiver,slot) in expected.items()],buffer_fixture=buffers,owner_fixture_minimum_bytes='0x2870',capability_fixture_minimum_bytes='0x540',owner_command_state_fields=dict(device='0x648',queued='0x658',cursor='0x660',remaining='0x664'),queued_constant_fixture_words=4096,tail_argument_bits='0x40000000',requirements=['Private original owner uses native32 offsets; x64 owner remains normalized.', 'Remap all22 globals and32 relative calls; bind all8 indirect calls.', 'Record allocation/free event order, complete buffer contents, queue contents/cursors and tail calls.', 'Original nested methods must either execute their own isolated verified bodies or be explicitly labelled controlled callbacks.', 'Do not execute allocation failure cases until unsafe null-write paths are isolated.'],native_executed=False,limits='Complete external binding inventory for this helper; does not prove full execution, nested runtime integration, GPU behavior or owned failure handling.')
Path('outputs/sfm64-constant-restore-comparison-bindings.json').write_text(json.dumps(report,indent=2)+'\n')
print('constant_restore_bindings=32direct+22globals+8indirect buffers=12 verified')
