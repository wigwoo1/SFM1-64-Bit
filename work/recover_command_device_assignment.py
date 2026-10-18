from pathlib import Path
import re,json,hashlib
from pe_image import Image
im=Image(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines()
regions=[(0x1002a670,0x1002a7bc),(0x1002a8d3,0x1002a8e6),(0x1002a8fb,0x1002a911),
         (0x100290a0,0x100290dd),(0x100290e0,0x10029101),(0x100568ac,0x100568b2),(0x100f9690,0x100f96c1)]
verified=[]
for start,end in regions:
    covered={};address=None
    for line in lines:
        m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)',line)
        if m:address=int(m[1],16);raw=bytes.fromhex(m[2])
        elif address is not None and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):raw=bytes.fromhex(line)
        else:continue
        for value in raw:
            if start<=address<end:covered[address]=value
            address+=1
    assert len(covered)==end-start
    raw=bytes(covered[x] for x in range(start,end));offset=im.offset(start)
    assert im.b[offset:offset+len(raw)]==raw
    verified.append({'start':hex(start),'end':hex(end),'bytes':len(raw)})
report={'dll_sha256':hashlib.sha256(im.b).hexdigest(),'verified_regions':verified,
 'flow':['Creation helper1002a670 reads service1016dfb8+30, invokes COM slot16 with output pointer/local and presentation-state owner+30; successful nonnull output returns as EAX.',
         'Caller1002a8db stores helper return in EBX; null takes failure return. Nonnull calls1002a610 then getter1000df50;1002a90f stores EBX to returned command-state device slot0.',
         'Caller creation flags include bit2 through OR EBX,2 before helper; full flag policy and service identity remain separately unverified.'],
 'device_slot_assignment_identified':True,'d3d9_create_device_identity_inferred_from_com_slot_and_arguments':True,
 'queue_activation_identified':False,'runtime_execution_verified':False,
 'limits':'Selected instruction ranges verified, not full creation/setup function execution. COM interface provenance, pointer lifetime, release/reset paths, queue enable and worker/consumer setup remain unfinished.'}
assert im.word(0x1016dfb8)==0x10183bb0
assert im.string(im.base+im.word(0x100fa1cc)+2)=='Direct3DCreate9'
report['service_object']='0x10183bb0'
report['factory_import']='Direct3DCreate9'
report['factory_sdk_version']=32
report['d3d9_interface_factory_provenance_verified']=True
report['flow'].append('Service global1016dfb8 initially points to10183bb0. Startup100f9690 installs vtable100fdc24 and clears object+30. Setup100290a0 calls Direct3DCreate9(32) through thunk100568ac/IAT100fa1cc and stores result+30. Release100290e0 calls interface slot2 then clears+30.')
report['flow'].append('Full creation helper1002a670..1002a7bc: successful HRESULT plus nonnull output returns. Initial failure sleeps1000ms then retries same arguments. Second failure checks live owner+50; when nonzero sets presentation+18 (owner+48) to3 and presentation+0c (owner+3c) to0 before third attempt. Final failure optionally follows external window/dialog policy, then returnsnull. Output local startsnull and is not explicitly cleared between attempts.')
report['full_creation_helper_static_bytes']=332
report['fallback_presentation_fields']={'native_owner_48':'D3DPRESENT_PARAMETERS.SwapEffect = 3 (COPY)','native_owner_3c':'D3DPRESENT_PARAMETERS.BackBufferCount = 0','native_owner_50':'D3DPRESENT_PARAMETERS.Windowed controls fallback, native32 layout'}
assert im.string(0x100fe6b8)=='mat_queue_mode'
assert im.string(im.base+im.word(0x100fa1fc)+2)=='GetCPUInformation'
report['creation_flag_selection']={'config':'mat_queue_mode','cpu_import':'GetCPUInformation','condition':'mode2, or mode-2 with CPU byte6>=2, or reloaded mode-1 with CPU byte6>=2 adds flag4; CPU byte meaning unresolved'}
report['limits']='Selected setup/assignment/factory/release ranges and import name verified statically. Full creation retry/configuration behavior, runtime execution, device reset/release ownership, queue activation and consumer setup remain unfinished.'
Path('outputs/sfm64-command-device-assignment-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_assignment_path_bytes='+str(sum(x['bytes'] for x in verified)))
