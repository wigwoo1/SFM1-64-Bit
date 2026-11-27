"""Compose original welcome entry, observer controller, solid setter and admission."""
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
    image,evidence=verify_installed_ranges(SERVER,run/'welcome-solidity-reference.local.txt',[
        ('full TF player spawn static evidence',0x103222d0,0x10322832),
        ('welcome entry special/global3 path',0x1031b300,0x1031b38e),
        ('observer controller',0x101c5090,0x101c52b0),
        ('solid flags setter',0x100f13e0,0x100f147c),
        ('collision flag refresh',0x100f0550,0x100f05fe),
        ('actor trigger admission',0x100cddb0,0x100cdec5),
        ('spawn scoreboard bookkeeping',0x10300460,0x103004da),
        ('spawn shared vector cleanup',0x1031b910,0x1031b970),
        ('spawn timed handle cleanup',0x10319890,0x10319904),
        ('spawn FOV handle reset',0x100ddb70,0x100ddbd9),
        ('spawn class view lookup',0x103246a0,0x103246da),
        ('actual player view offset setter',0x100ce660,0x100ce6c8),
        ('actual surrounding bounds setup',0x100f1630,0x100f172f),
    ])
    assert image.word(0x1050eaac+0x60)==0x103222d0
    assert image.word(0x1050eaac+0x630)==0x101c5090
    spawn=evidence[0]['instructions']
    assert any(row['address']==0x103223b0 and row['instruction']=='call        1031B300' for row in spawn)
    exe=directory/'welcome_x86.exe';log=(directory/'x86-build.log').read_text(errors='replace')
    assert 'warning C' not in log and 'error C' not in log,log
    data=exe.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
    assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==0x14c
    for repeat in (1,2):
        result=subprocess.run([str(exe),str(SERVER)],capture_output=True,text=True,timeout=45)
        (run/f'x86-{repeat}.log').write_text(result.stdout+result.stderr)
        assert result.returncode==0,(result.returncode,result.stderr)
        assert json.loads(result.stdout)==dict(cases=32,welcome_calls_per_case=2,suffix_calls_per_case=2,passed=True)
    assert sha(SERVER)==REFERENCES[SERVER]
    receipt=dict(status='VERIFIED BOUNDED WELCOME AND COMMON SPAWN SUFFIX',cases_per_run=32,welcome_calls_per_case=2,suffix_calls_per_case=2,repeats=2,
        executable=str(exe),sha256=sha(exe),machine='0x14c',warnings=0,server_unchanged=True,
        server_sha256=sha(SERVER),verified_spans=[row['name'] for row in evidence],
        findings=['Installed Spawn calls welcome entry1031b300 when actor state15D4 is1. Full Spawn is static evidence only.',
                  'Actual welcome entry, empty observer search, observer controller, solid flag setter and its no-partition refresh execute together.',
                  'Welcome requests observer mode3 through a supplied mode service, requests movement0 through an observer, adds flag4 and effects0x30, then takes the global48=3/panel branch.',
                  'Ordinary initial flags0/4 become4 and request no engine trigger update after a disposable edict is attached. Initial flags8/12 retain trigger bit8 and request engine trigger update.',
                  'Whole actor storage after two welcome calls matches expected controller/welcome field updates. Repeated welcome calls keep the same tested state.'],
        limits=['Original x86 composition only. No production x64 welcome controller or editor integration is added.',
                'Full Spawn prefix, active-state branch, allocating/statistics cases and live actor position/state are unverified. The common suffix is entered through an explicit original-stack/register harness.',
                'Observer mode setter, direct movement setter, entity flag/effect notifications and entity lookup are adapters; observer lookup returns no points.',
                'No weapon, ground or physics object; clean transform; observer physics bit8 already set; no edict during welcome; collision partition handleFFFF and context membership0.',
                'Game rule query/panel/bounds/end callbacks, FOV reset100ddbe0 and engine trigger-update callback are supplied. Post-welcome admission uses a disposable edict and pre-set update-list mark.',
                'Actual overlaps, trigger outputs and door behavior remain unverified; prior trigger bit8 cases are test inputs, not claims about live player state.'])
    receipt['findings'] += [
        'Original common Spawn suffix1032269b..10322832 executes twice after welcome, including original no-scoreboard bookkeeping, empty shared vector cleanup, absent handle cleanup, class view lookup, view setter and surrounding-bounds mode3 setup.',
        'Solidity and cached world origin280 are unchanged by the tested common suffix. Class1/10 view offsets are stored at358; supplied rule bounds are stored as surrounding bounds.',
        'Spawn byte1780 toggles each call; tested timestamps update and suffix engine end callback runs. View setter entry observations execute through original body using CPU resume flag.']
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases=32,repeats=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
