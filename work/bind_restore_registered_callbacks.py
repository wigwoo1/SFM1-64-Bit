
from pathlib import Path
import json,struct
exec(Path('work/audit_restore_queue_counter_references.py').read_text().split('selected=[]')[0])
ranges=[('extended_constants',0x10011610,0x10011636),('viewport',0x10019910,0x10019a09),('vertex_textures',0x1001f8a0,0x1001fb8f)]
bodies=[]
for name,begin,end in ranges:
 selected=[r for r in rows if begin<=int(r['address'],16)<end];cursor=begin;calls=[]
 for row in selected:
  a=int(row['address'],16);raw=bytes.fromhex(row['bytes']);assert a==cursor and im.b[im.offset(a):im.offset(a)+len(raw)]==raw;cursor+=len(raw)
  if raw[0]==0xe8:calls.append({'address':hex(a),'target':hex(cursor+struct.unpack('<i',raw[1:])[0])})
 assert cursor==end and selected[-1]['bytes']=='c3'
 bodies.append({'name':name,'begin':hex(begin),'end':hex(end),'bytes':end-begin,'instructions':selected,'direct_calls':calls})
viewport_group=[r for r in rows if 0x10026f5e<=int(r['address'],16)<=0x10026f8f]
assert viewport_group[0]['bytes']=='33c9'
assert any(r['address']=='0x10026f8c' and r['bytes']=='51' for r in viewport_group)
for row in viewport_group:
 a=int(row['address'],16);raw=bytes.fromhex(row['bytes']);assert im.b[im.offset(a):im.offset(a)+len(raw)]==raw
 if a<0x10026f8c and a!=0x10026f5e:assert not row['instruction'].startswith(('mov         ecx,','xor         ecx,','lea         ecx,','pop         ecx','call'))
viewport=bodies[1]
assert any(r['address']=='0x10019978' and '7' in r['instruction'] for r in viewport['instructions'])
assert any(r['address']=='0x10019985' and '10180AA8h' in r['instruction'] for r in viewport['instructions'])
report={'sha256':sha,'verified_callback_bodies':bodies,'viewport_registration_kind0_proof':viewport_group,
 'bindings':[
 {'callback':'0x10011610','group':0,'role':'Wrapper submits constant registers1024..1535 through1000f1b0, using supplied command state in EAX.','existing_policy':'work/studio_extended_constants_callback_candidate.h','existing_fixture':'work/test_restore_extended_dispatch_native.cpp','remaining':'Original dispatch loop/wrapper/helper and x64 owned registry/wrapper/helper have a composed fixture; complete tail gates/cleanup/snapshot, real refill/device and full restore integration pending.'},
 {'callback':'0x10019910','group':0,'role':'Six-DWORD viewport comparison; force low byte bypasses equality; zero width/height suppress. Queue opcode20 or device slot47 through static command-state10180aa8, ignoring supplied queue argument.','existing_policy':'work/studio_shader_viewport_submit_candidate.h','existing_fixture':'work/test_shader_viewport_submit.cpp','remaining':'Real static device/queue binding and combined restore execution pending.'},
 {'callback':'0x1001f8a0','group':1,'role':'External service1019092c slot31 supplies count; texture/sampler restoration in24-byte current/cache records through supplied queue or device slots65/69.','existing_policy':'work/studio_vertex_texture_submit_candidate.h','existing_fixture':'work/test_vertex_texture_callback_native.cpp','remaining':'Real count service, resource lifetime, fullowner callback dispatch and queue integration pending.'}],
 'finding':'Identified registered callbacks restore render state; inspected bodies do not directly advance recovered global completion counter or set queue enable. Viewport callback introduces a second command-state object sharing refill helper.',
 'limits':'Static body and registration group binding; existing fixtures are linked as separate evidence, not rerun or asserted wholecallback/fullrestore proof. No alias-complete negative writer proof or worker-thread synchronization established.'}
for binding in report['bindings']:
 for key in ['existing_policy','existing_fixture']:assert Path(binding[key]).is_file()
Path('outputs/sfm64-restore-registered-callback-bindings.json').write_text(json.dumps(report,indent=2)+chr(10))
print('registered_callback_bodies=3 verified_bytes='+str(sum(b['bytes'] for b in bodies))+' viewport_registration_kind0=verified')
