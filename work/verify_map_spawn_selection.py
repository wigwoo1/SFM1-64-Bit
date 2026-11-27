"""Compare installed team/world spawn selection with an owned x64 service policy."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_map_trigger_actor import SERVER, REFERENCES, sha
from verify_sliding_door_spawn import verify_installed_ranges


def main(directory):
    assert sha(SERVER)==REFERENCES[SERVER], 'Unrecognized installed server'
    run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    image,evidence=verify_installed_ranges(SERVER,run/'spawn-selection-reference.local.txt',[
        ('actual spawn entity selector',0x10317d70,0x10317eea),
        ('hull minimum query - not position',0x100db4e0,0x100db562),
        ('hull maximum query - not position',0x100db570,0x100db5f2),
        ('hull application forwarding',0x100b4110,0x100b411f),
        ('base spawn game rules placement dispatch',0x101cc2c3,0x101cc2ee),
        ('base spawn clears observer physics flags',0x101cc1ea,0x101cc22d),
        ('game rules selects entity and traces placement',0x102fb050,0x102fb13d),
    ])
    player=0x1050eaac
    assert image.word(player+0x68c)==0x10317d70
    assert image.word(player+0x5a0)==0x100db4e0 and image.word(player+0x5a4)==0x100db570
    rules=0x10506404;locator=image.word(rules-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCTFGameRules@@'
    assert image.word(rules+0x124)==0x102fb050
    builds={}
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        exe=directory/f'selection_{arch}.exe';log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=exe.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            result=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-'],capture_output=True,text=True,timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.returncode,result.stderr)
            assert json.loads(result.stdout)==dict(cases=480,passed=True)
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED SPAWN SELECTION',cases_per_run=480,repeats_per_arch=2,builds=builds,
        server_unchanged=True,server_sha256=sha(SERVER),verified_spans=[row['name'] for row in evidence],
        findings=['Actual selector resolves a per-team recent-spawn handle before branching. Teams2/3 call supplied info_player_teamspawn selection; teams0/1 use the global world edict/entity.',
                  'Successful team lookup updates its cached handle; failed lookup can still return a retained/nonzero selection. Typed spawn field17FC is updated on the team path only.',
                  'Missing selection emits a classname-specific warning and resolves world entity again. Missing/free edict, absent networkable and null entity are covered.',
                  'Unrelated player bytes and other team cache slots are unchanged. Service ordering and results match the x64 helper.',
                  'Static correction: player virtual5A0/5A4 obtain rule hull min/max by observer/duck flags; 100B4110 forwards these to collision hull setup. They are not position queries.',
                  'Static base Spawn clears C44 before rule virtual124; CTFGameRules124 calls player selector68C, traces from selected entity origin and applies the trace endpoint. These placement routines are inspected, not executed.'],
        limits=['Team spawn chooser, dynamic type check, warning and entity/networkable services are supplied callbacks. Native handle serial checks and selector branches execute.',
                'Only team indices0..3 and stable service objects are covered; live team, world transform, spawn validity and collision results are not established.',
                'Game-rules placement, ray trace10019790, origin application and full Spawn do not execute. No claim about live mtt_soldier player coordinates.',
                'Owned helper preserves service policy only and is isolated from editor actors. No camera-based fallback is introduced.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=480,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
