"""Compare the installed valid-player activation sequence with owned x64 policy.

Spawn implementations, engine string tables and announcement delivery are test
adapters. This is not a live-player or spawn-position acceptance test.
"""
import argparse
import json
import re
from pathlib import Path
import struct
import subprocess
import tempfile

from verify_map_trigger_actor import SERVER,REFERENCES,sha
from verify_sliding_door_spawn import BSP,verify_installed_ranges


def main(directory):
    assert sha(SERVER)==REFERENCES[SERVER], 'Unrecognized installed server'
    bsp_hash=sha(BSP)
    run=Path(tempfile.mkdtemp(prefix='verify-',dir=directory))
    image,evidence=verify_installed_ranges(SERVER,run/'activation-reference.local.txt',[
        ('valid-player activation resolver',0x102f19e0,0x102f1a34),
        ('finish activation',0x102f17f0,0x102f18d6),
        ('extra client flag getter',0x101baed0,0x101baed7),
        ('TF initial spawn',0x10322250,0x103222ce),
        ('player state transition',0x10321ad0,0x10321b66),
        ('installed state lookup and initialization',0x10321850,0x10321ac5),
        ('welcome entry observer prefix',0x1031b300,0x1031b322),
        ('welcome observer search',0x10314f50,0x10314fb0),
    ])
    table=0x1050eaac
    locator=image.word(table-4)
    assert image.word(locator+4)==0 and image.string(image.word(locator+12)+8)=='.?AVCTFPlayer@@'
    assert image.word(table+0x56c)==0x10322250 and image.word(table+0x60)==0x103222d0
    assert image.string(0x10503698)=='<unconnected>' and image.string(0x10503688)=='#Game_connected'
    assert image.string(0x1050f470)=='TF_STATE_WELCOME'
    assert image.string(0x1050721c)=='info_observer_point'
    bsp=BSP.read_bytes();assert bsp[:4]==b'VBSP'
    start,size=struct.unpack_from('<2I',bsp,8);assert start+size<=len(bsp)
    classes=[dict(re.findall(rb'"([^"]+)"\s*"([^"]*)"',entry)).get(b'classname')
             for entry in re.findall(rb'\{[^{}]+\}',bsp[start:start+size])]
    observer_points=classes.count(b'info_observer_point')
    assert observer_points==0
    builds={};results=[]
    for arch,machine in [('x86',0x14c),('x64',0x8664)]:
        executable=directory/f'activation_{arch}.exe'
        log=(directory/f'{arch}-build.log').read_text(errors='replace')
        assert 'warning C' not in log and 'error C' not in log,log
        data=executable.read_bytes();pe=struct.unpack_from('<I',data,60)[0]
        assert data[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',data,pe+4)[0]==machine
        builds[arch]=dict(executable=str(executable.resolve()),sha256=sha(executable),machine=hex(machine),warnings=0)
        for repeat in (1,2):
            p=subprocess.run([str(executable),str(SERVER) if arch=='x86' else '-'],capture_output=True,text=True,timeout=40)
            (run/f'{arch}-{repeat}.log').write_text(p.stdout+p.stderr)
            assert p.returncode==0,(arch,p.returncode,p.stderr)
            result=json.loads(p.stdout)
            assert result==dict(cases=96,initial_cases=108 if arch=='x86' else 0,observer_cases=1364,passed=True)
            results.append(result)
    assert all(result['cases']==results[0]['cases'] and result['passed'] for result in results)
    assert sha(SERVER)==REFERENCES[SERVER]
    assert sha(BSP)==bsp_hash
    receipt=dict(status='VERIFIED BOUNDED VALID-PLAYER ACTIVATION',cases_per_run=96,repeats_per_arch=2,
        builds=builds,server_unchanged=True,server_sha256=sha(SERVER),
        original_initial_spawn_cases_per_run=108,benchmark_bsp_sha256=bsp_hash,
        observer_selection_cases_per_arch=1364,
        authored_observer_points=observer_points,
        verified_spans=[row['name'] for row in evidence],
        context_source='https://raw.githubusercontent.com/ValveSoftware/source-sdk-2013/master/src/game/server/tf/tf_client.cpp',
        findings=['Original valid-player resolver reaches initial-spawn then spawn while the supplied string-table service is unlocked, then restores its previous lock state.',
                  'Both supplied load-game booleans take the same tested default TF activation path; this does not establish load-game support.',
                  'The name is prepared before fake-client notification gating, percent characters are replaced with spaces, and an empty name uses <unconnected>.',
                  'Fake clients suppress the announcement unless the separately tested player byte at 126c is set; its semantic name is not assumed.',
                  'Additional original-x86 initial-spawn cases execute the state lookup/transition and reach the installed TF_STATE_WELCOME entry, including reentry when already in state 1.',
                  'The welcome entry prefix calls the observer-point search. The installed benchmark BSP has no authored info_observer_point entities.',
                  'The executed observer search enumerates supplied points in order, selects every accepted point, then tests the current stop flag. No points or no accepted points preserve the supplied prior selection.',
                  'A stop flag on a rejected point still ends the search; target callbacks may change the stop flag before it is tested.'],
        limits=['Only a valid explicit edict/player is tested; missing/fallback entities and the full CServerGameClients outer wrapper are outside the dynamic matrix.',
                'Initial-spawn and spawn virtual bodies are supplied observers. Actual CTFPlayer construction, class/team selection, collision setup and spawn position are not implemented here.',
                'String-table service and name-copy primitive are adapters; original string-table/CRT internals are not executed.',
                'Notification is observed at its entry; original message/network delivery is not executed.',
                'The helper has no exception-restoration claim and is not integrated into the complete editor.',
                'Public TF source provides context, not proof of current SFM binary equivalence.'])
    receipt['limits'].extend([
        'Initial-spawn extension is x86-only evidence: base/shared initialization, virtual slot 748 and welcome-entry body are supplied observers. Optional feature gate is disabled and transition logging suppressed.',
        'Observer search executes in the reference and owned x64 helper using supplied entity enumeration, eligibility and target callbacks. It does not validate actual game target eligibility or move a player.',
        'Welcome entry, observer-mode implementation, full Spawn body, runtime-created observer entities and later player movement remain unverified.',
        'Authored observer-point absence does not determine a live player position or prove absence of door touches.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(cases_per_arch=96,observer_cases_per_arch=1364,original_initial_spawn_cases=108,repeats_per_arch=2,receipt=str(run/'results.json'))))


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
