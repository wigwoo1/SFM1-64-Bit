from pathlib import Path
import json
exec(Path('work/recover_device_manager_allocation_owner.py').read_text().split('connect=region')[0])
services=[]
for i in range(56):
 address=0x1017b158+8*i;name=im.word(address);cell=im.word(address+4)
 if cell in [0x10190904,0x101908e4]:services.append(dict(index=i,table_address=hex(address),name_address=hex(name),name=im.string(name),output_cell=hex(cell)))
assert len(services)==2
recursive=region(0x100d8570,0x100d860e)
free_thunk=region(0x100039b0,0x100039c7)
print(services);print(lookup[0x100fa274]);print(free_thunk)
report=dict(sha256=sha,service_table_bindings=services,owned_recursive_cleanup=recursive,owned_allocator_import=lookup[0x100fa274],free_thunk=free_thunk,payload_allocator_import=lookup[0x100fa238],finding='Shutdown service identities resolved from installed name/output-cell table rather than naming virtual calls by guesswork. Owned cleanup recursively visits node+18 child chain and node+14 peer chain, detaches peer links before recursive cleanup, releases descendant nodes through imported service virtual offset8, and frees payloads at+4/+8 before clearing them. Recursive graph invariants and alias/cycle behavior are unverified.',limits='Fresh offline bytes/table/import proof; virtual method names and real runtime owners are not verified through execution. Do not substitute generic delete for imported ownership services.')
Path('outputs/sfm64-device-manager-cleanup-services.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_owned_recursive_cleanup_bytes='+str(recursive['bytes']+free_thunk['bytes']))

