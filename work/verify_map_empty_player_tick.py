"""Verify the original player's empty-command tick paths against owned x64 policy."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_map_trigger_actor import SERVER, REFERENCES, sha
from verify_sliding_door_spawn import verify_installed_ranges
from verify_map_playhead_listeners import imported_symbol


def main(directory):
    assert sha(SERVER)==REFERENCES[SERVER], 'Unrecognized installed server'
    run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    image,evidence=verify_installed_ranges(SERVER,run/'empty-player-reference.local.txt',[
        ('installed player PhysicsSimulate',0x101cd600,0x101cdb30),
        ('queued command tick count',0x101bed20,0x101bed6e),
        ('command context clear',0x101cd2d0,0x101cd38e),
        ('alternate tick predicate',0x100cdd90,0x100cddad),
    ])
    table=0x1050eaac;locator=image.word(table-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCTFPlayer@@'
    assert image.word(table+0x1b0)==0x101cd600
    assert image.string(0x104bcee4)=='CBasePlayer::PhysicsSimulate'
    assert imported_symbol(image,0x1043c250)==('tier0.dll','?EnterScope@CVProfile@@QAEXPBDH0_NH@Z')
    assert imported_symbol(image,0x1043c248)==('tier0.dll','?ExitScope@CVProfile@@QAEXXZ')
    builds={}
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        exe=directory/f'empty_{arch}.exe'
        log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=exe.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            result=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-'],capture_output=True,text=True,timeout=45)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.returncode,result.stderr)
            assert json.loads(result.stdout)==dict(cases=96,passed=True)
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED EMPTY PLAYER TICK',cases_per_run=96,calls_per_case=2,repeats_per_arch=2,builds=builds,
        server_unchanged=True,server_sha256=sha(SERVER),verified_spans=[row['name'] for row in evidence],
        findings=['Installed CTFPlayer simulation virtual is CBasePlayer::PhysicsSimulate at 101cd600.',
                  'A serial-valid nonnull parent simulates before the player same-tick guard, including repeat calls. Stale or null parent references do not run it.',
                  'A parent callback that marks the player already simulated on this tick suppresses its subsequent work.',
                  'On a new tick the last-tick field updates, queued ticks are counted and empty contexts are cleared. Player byte BC8 selects an additional supplied service at 101c5650.',
                  'With BC8 zero and no queued commands, the full routine returns without command processor lookup at 101775f0. Actor fields other than last tick and all server globals remain unchanged in the matrix.'],
        limits=['No command contexts, commands, CF8 ancillary updates or engine-managed lifetime. Global max-client field is 2, avoiding alternate single-client tick mode.',
                'Parent simulation and profiling imports are supplied callbacks. Special processing at 101c5650 is intercepted and does not execute.',
                'Count/clear entry observations set CPU resume flag and allow the actual original callees to execute; they are not substituted.',
                'The owned x64 helper has an explicit empty-queue precondition; it is not a full player simulator and is not integrated into the editor.',
                'Queued-command processing, actual movement/collision, spatial relinking, map touches and door activation remain unverified. No claim about the live SFM player queue/state.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=96,calls_per_case=2,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
