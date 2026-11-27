"""Compare actual installed list mutations and due collection with owned x64 policy."""
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
    image,evidence=verify_installed_ranges(SERVER,run/'admission-reference.local.txt',[
        ('entity simulation list refresh',0x10110c70,0x10110d71),
        ('spare-capacity list append',0x10110be0,0x10110c2c),
        ('fast list removal',0x10110330,0x10110399),
        ('earliest think tick',0x100cd160,0x100cd19d),
        ('due entity collection',0x1010f430,0x1010f49b),
        ('due collection wrapper',0x1010f4a0,0x1010f4b7),
        ('simulation list dispatch loop',0x1019745d,0x101974c4),
        ('per-entity simulation dispatch',0x10197330,0x101973b0),
    ])
    assert any('call        1010F4A0' in row['instruction'] for row in evidence[6]['instructions'])
    assert any('call        10197330' in row['instruction'] for row in evidence[6]['instructions'])
    builds={}
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        exe=directory/f'admission_{arch}.exe'
        log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=exe.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine
        builds[arch]=dict(executable=str(exe),sha256=sha(exe),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            result=subprocess.run([str(exe),str(SERVER) if arch=='x86' else '-'],capture_output=True,text=True,timeout=90)
            (run/f'{arch}-{repeat}.log').write_text(result.stdout+result.stderr)
            assert result.returncode==0,(arch,result.returncode,result.stderr)
            assert json.loads(result.stdout)==dict(cases=11520,passed=True)
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED SIMULATION ADMISSION',cases_per_run=11520,repeats_per_arch=2,builds=builds,
        server_unchanged=True,server_sha256=sha(SERVER),verified_spans=[row['name'] for row in evidence],
        findings=['Removal flag bit0 and invalid handles skip all list mutation.',
                  'Both exclusion flags (bits22 and23) remove an existing entry by swapping the final entry into its slot and repairing the index map.',
                  'Otherwise missing entries append. Physics-enabled entries store tick0; physics-excluded entries store the earliest positive base/context think tick, or -1.',
                  'Due collection checks the first min(capacity,list size) entries and copies those with stored tick <= current tick. It retains null resolved pointers and preserves list order.',
                  'Installed simulation dispatch calls the due wrapper, skips null entries and passes each remaining entity to 10197330. This consumer connection is static evidence only.'],
        limits=['Original mutation, spare-capacity append, swap removal, earliest-tick calculation and collector execute without intercepted helper bodies.',
                'Entity handle getter and global entity-pointer table are supplied fixture data; handle lifetime, stale serial references and engine ownership are not verified.',
                'Growth beyond spare capacity, allocation failure and malformed manager state are not covered.',
                'Consumer loop and per-entity simulation body were inspected but not executed; physics, relinking, trigger contact and door activation remain unverified.',
                'The helper is isolated and not wired into the editor. It is not a full simulation scheduler.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=11520,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
