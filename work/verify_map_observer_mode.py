"""Execute the installed observer-mode controller under declared service adapters.

Private disassembly and receipts stay in the supplied disposable directory.
This original-x86 fixture does not supply a production x64 observer runtime.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_map_trigger_actor import SERVER,REFERENCES,sha
from verify_sliding_door_spawn import verify_installed_ranges


def main(directory):
    assert sha(SERVER)==REFERENCES[SERVER], 'Unrecognized installed server'
    run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    image,evidence=verify_installed_ranges(SERVER,run/'observer-mode-reference.local.txt',[
        ('observer-mode controller',0x101c5090,0x101c52b0),
        ('empty active weapon resolution',0x100ab6a0,0x100ab6cc),
        ('remove player flags',0x100ce730,0x100ce785),
        ('add effect bits',0x100cea00,0x100cea56),
    ])
    table=0x1050eaac;locator=image.word(table-4)
    assert image.string(image.word(locator+12)+8)=='.?AVCTFPlayer@@'
    assert image.word(locator+4)==0 and image.word(table+0x630)==0x101c5090
    executable=directory/'observer_x86.exe'
    log=(directory/'x86-build.log').read_text(errors='replace')
    assert 'warning C' not in log and 'error C' not in log,log
    data=executable.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
    assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==0x14c
    for repeat in (1,2):
        result=subprocess.run([str(executable),str(SERVER)],capture_output=True,text=True,timeout=40)
        (run/f'run-{repeat}.log').write_text(result.stdout+result.stderr)
        assert result.returncode==0,(result.returncode,result.stderr)
        assert json.loads(result.stdout)==dict(cases=96,calls_per_case=2,passed=True)
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED ORIGINAL OBSERVER CONTROLLER',cases_per_run=96,calls_per_case=2,repeats=2,
        executable=str(executable.resolve()),executable_sha256=sha(executable),compiler_warnings=0,
        reference_sha256=sha(SERVER),original_unchanged=True,verified_spans=[row['name'] for row in evidence],
        findings=['With bit 8 clear at player+c44, the controller adds the supplied view offset to clean cached origin and requests zero view offset.',
                  'It sets bit 8, so the second call does not translate the origin again.',
                  'Under the tested empty-weapon/no-ground domain, it removes player flag 2, requests solid flags OR 4, adds effect bit 32, requests mode 3 or 6 and writes the checked observer-entry fields.',
                  'The controller returns true even when the supplied mode-setting virtual returns false.'],
        limits=['Original x86 only. This does not implement or verify an owned x64 controller or editor integration.',
                'Origin and solid-flag setters, entity-list flag notification, effect invalidation, view getters/setters, mode setting and network-field notifications are adapters.',
                'Clean cached transforms only; dirty-parent transform resolution, active weapon handling and prior ground-entity removal are outside the matrix.',
                'Player byte 9c1 is zero and global word 48 equals 3: optional game-rule and viewport-panel paths are not executed.',
                'Modes 3 and 6 are requested, but the real mode setter, collision relinking, observer targets, physics and map touches do not run.',
                'Field-write verification does not establish the live actor state or position during an SFM session.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases=96,calls_per_case=2,repeats=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
