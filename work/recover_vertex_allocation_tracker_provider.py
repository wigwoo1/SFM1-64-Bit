from pathlib import Path
import json
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
startup=region(0x100f9170,0x100f9185)
factory=region(0x10002cb0,0x10002cb6)
linker=region(0x100d5fb0,0x100d5fd2)
register=region(0x10002cc0,0x10002cc3)
other=region(0x10007180,0x10007183)
assert im.string(0x100fa41c)=='VBAllocTracker001'
assert im.word(0x1016d048)==0x100fa410
slots=[im.word(0x100fa410+4*i) for i in range(3)]
assert slots==[0x10002cc0,0x10007180,0x10007180]
assert bytes.fromhex(register['instructions'][0]['bytes'])==bytes.fromhex('c21800')
assert bytes.fromhex(other['instructions'][0]['bytes'])==bytes.fromhex('c20400')
report=dict(sha256=sha,interface='VBAllocTracker001',startup=startup,factory=factory,registration_linker=linker,object='0x1016d048',vtable='0x100fa410',slots=list(map(hex,slots)),slot_bodies=[register,other],
finding='Local startup registers interface name and factory10002cb0 in node1017e00c via linked-list constructor100d5fb0/head1019fbd8. Factory returns staticobject1016d048 withthree-slot vtable. Slot0 RET18 andslots1/2 RET4 performno work; registration isno-op for this localprovider.',
limits='Static installed-byte proof of localprovider, not runtime selection. Global10190934 binding can use another module factory; connector ordering/selection unresolved. No GPU resource lifetime inferred from no-op tracker.',
next_action='Use verified local no-op tracker in constructor composition, then trace connector selection before choosing application default tracker.')
Path('outputs/sfm64-vertex-allocation-tracker-provider.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_local_tracker_provider_bytes='+str(sum(x['bytes'] for x in [startup,factory,linker,register,other]))+' slots=3 no_op=True')
