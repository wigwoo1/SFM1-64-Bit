from pathlib import Path
import json
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
regions=[region(0x100ddf80,0x100ddfca),region(0x100de000,0x100de027),region(0x100de030,0x100de05e)]
report=dict(sha256=sha,regions=regions,finding='ResourceConnect first conditionally initializes five precache groups (provider1019094c, flag101b11fd). Group loop reloads provider each iteration, invokes slot10 with group array entry and index. Sets flag after loop. Then if resource10190904 nonnull and flag101b11fc false, invokes slot8 token101b1200, sets flagtrue andtailcalls listregistration. Listhead101b122c cached once; eachnode invokes reloaded resource provider slot12, then reloads node+0c next after callback. Alreadyregistered resource skips listregistration.',limits='Fresh static bytes. Group tokens/node layout and real virtualmethod semantics provisional; callback cycles/null provider mutations are not validated.')
Path('outputs/sfm64-resource-startup-registration.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_resource_startup_registration_bytes='+str(sum(x['bytes'] for x in regions)))
