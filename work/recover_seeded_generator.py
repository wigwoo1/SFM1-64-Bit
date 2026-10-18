from pathlib import Path
import json,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
body=region(0x100d5eb0,0x100d5fa7);assert not body['direct_calls']
scale=struct.unpack_from('<d',im.b,im.offset(0x101526e8))[0]
assert scale==1/65536
start=0x10198eb0-0x370;count=(0x1019feb0-0x10198eb0)//0x380
assert count==32
report=dict(sha256=sha,body=body,scale=scale,scale_bytes=im.b[im.offset(0x101526e8):im.offset(0x101526e8)+8].hex(),state_start=hex(start),blocks=count,block_bytes=896,values_per_block=220,pointer_fields=[dict(offset=880,value_index=92),dict(offset=884,value_index=216)],preserved_tail_bytes=8,recurrence='For each block seed+block modulo32; store float((state>>16)/65536); state=0xbb40e62d-state*0x44bf19d3 modulo32; repeat220. Two pointers initialized before values. Final8bytes perblock untouched.',limits='Structural recovery. Float rounding and original execution not yet verified; generator stepping and thread ownership remain separate.')
Path('outputs/sfm64-seeded-generator-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_seed_init_bytes=247 blocks=32 values=7040 scale='+str(scale))
