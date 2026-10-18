from pathlib import Path
import json,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
parts={'base_disconnect':region(0x10027c50,0x10027c8b),'interface_disconnect_thunk':region(0x100dbe20,0x100dbe25),'interface_disconnect':region(0x100d4db0,0x100d4ded),'service_cleanup_a':region(0x100ddfd0,0x100ddff7),'service_cleanup_b':region(0x100d6120,0x100d6169),'owned_cleanup':region(0x100d9760,0x100d977e)}
report=dict(sha256=sha,regions=parts,finding='Base disconnect clears globals10183b54/58, calls cleanup100ddfd0 then100d6120 then100dbe20 (tail thunk to interface disconnect100d4db0), then if receiver+2c is nonnull calls100d9760 and clears field. Interface disconnect decrements generation modulo32; if signed negative returns, otherwise if signed tracked count positive clears output cells only for records tagged with decremented generation. Does not shrink tracked count or alter records. Service cleanup virtual semantics and receiver+2c ownership remain provisional.',limits='Static fresh-byte evidence. No original base cleanup execution or complete external service/lifetime proof.')
Path('outputs/sfm64-device-manager-base-disconnect.json').write_text(json.dumps(report,indent=2)+'\n')
b=parts['interface_disconnect'];raw=b''.join(bytes.fromhex(x['bytes']) for x in b['instructions']);fixups=[]
for value in [0x101909c4,0x101909c8,0x1019071c,0x10190718]:
 needle=struct.pack('<I',value);start=0
 while True:
  pos=raw.find(needle,start)
  if pos<0:break
  fixups.append((pos,value));start=pos+4
assert len(fixups)==5
Path('work/interface_disconnect_native_init.h').write_text('#pragma once\nstatic const unsigned disconnectOffset='+str(im.offset(int(b['start'],16)))+';\nstatic const unsigned char disconnectBytes[]={'+','.join(hex(x) for x in raw)+'};\nstatic const unsigned disconnectFixups[][2]={'+','.join('{'+str(p)+','+hex(v)+'}' for p,v in fixups)+'};\n')
print('verified_base_disconnect_chain_bytes='+str(sum(x['bytes'] for x in parts.values())))
