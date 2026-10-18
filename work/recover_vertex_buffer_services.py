from pathlib import Path
import json
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
copy=region(0x100d7470,0x100d748c)
append=region(0x100d7640,0x100d769d)
vertex=region(0x10005570,0x10005765)
assert copy['direct_calls']==[]
assert [x['target'] for x in append['direct_calls']]==['0x100e5bb0']
labels={hex(a):im.string(a) for a in (0x100fb000,0x100fb020,0x100fb030)}
proof_addresses=[0x10005593,0x10005596,0x1000559c,0x100055a7,0x10005589,0x100055f4,0x10005634,0x100056b4,0x100056ea,0x100056ef,0x10005720,0x10005723,0x10005726,0x1000572b,0x1000574e,0x10005762]
by_address={int(x['address'],16):x for x in vertex['instructions']}
report=dict(sha256=sha,helpers=[copy,append],labels=labels,proofs=[by_address[a] for a in proof_addresses],
arguments={'stack_08':'command object','stack_0c_10':'opaque registration arguments','stack_14':'device FVF argument','stack_18':'vertex count','stack_1c':'stride','stack_20':'counter name suffix string','stack_24':'managed usage byte','stack_28':'dynamic low bit'},
receiver={'00':'resource pointer','04':'DWORD byte size count*stride','08':'zero','0c':'stride','10':'vertex count','14':'flags dynamic bit0/set bit2','18':'sentinel ffffffff','1c':'frame counter pointer for static buffers','20':'global or dynamic counter pointer','24_28':'zero'},
policy='Static buffers copy TexGroup_global_ then append supplied suffix within256-byte destination, find group2 counter; repeat with TexGroup_frame_ and group3. Dynamic buffers find fixed TexGroup_global_Dynamic Verts group2. Names are copied by profile factory. Initial size captured from live receiver+4 after counter services; retry reloads receiver+4 before its flush. Initial FVF stack14, usage and pool retained across retry. Static final accounting reloads receiver+4 and counter pointer+20. Final global10190934 vtable slot0 receives resource, dynamic bit, live bytes/count, opaque stack0c/10. RET24 confirms nine stack arguments.',
limits='Full helper and vertex bytes verified against installed image. Append memcpy-like target100e5bb0 not recovered here; full registration interface ownership and cookie service unresolved. No vertex constructor execution or GPU claim.',
next_action='Execute original constructor with verified string helpers, controlled append copy service, profile factory, registration interface and security-cookie service; compare pointerwide vertex policy including live retry size.')
Path('outputs/sfm64-vertex-buffer-services-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_vertex_service_bytes='+str(copy['bytes']+append['bytes']+vertex['bytes'])+' registration_arguments=9')
