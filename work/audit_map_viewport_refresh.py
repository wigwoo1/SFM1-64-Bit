"""Read-only audit of the viewport refresh boundary and benchmark spawn metadata.

Output includes private decoded reference instructions and must stay local.
This audit cannot prove the absence of transitive or Qt signal side effects.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import struct
import tempfile

from verify_map_playhead_listeners import imported_symbol
from verify_map_simulation_clock import IFM, REFERENCES, sha, verify_installed_ranges
from verify_sliding_door_spawn import BSP


def main(directory):
    before = {str(path):sha(path) for path in (IFM,BSP)}
    assert before[str(IFM)] == REFERENCES[IFM], 'Unrecognized installed IFM'
    run = Path(tempfile.mkdtemp(prefix='audit-',dir=directory))
    image, evidence = verify_installed_ranges(IFM,run/'viewport-refresh-reference.local.txt',[
        ('clip-change thunk',0x101afe80,0x101afe8b),
        ('viewport information refresh',0x101afc40,0x101afe72),
        ('viewport camera label refresh',0x101afa60,0x101afc3a),
        ('viewport flag getter',0x101a1a50,0x101a1a57),
        ('viewport mode getter',0x101191f0,0x101191f4),
    ])
    direct = {}
    imports = {}
    unresolved = {}
    for span in evidence:
        calls = []
        virtual = []
        for row in span['instructions']:
            instruction = row['instruction']
            match = re.fullmatch(r'call\s+([0-9A-F]{8})',instruction)
            if match:
                target = int(match[1],16)
                assert target != 0x10276790, 'Unexpected direct camera-to-player snap'
                calls.append(hex(target))
            elif instruction.startswith('call'):
                match = re.search(r'\[([0-9A-F]{8})h\]',instruction)
                if match:
                    address = int(match[1],16)
                    module,symbol = imported_symbol(image,address)
                    assert module in ('QtCore4.dll','QtGui4.dll'), (module,symbol)
                    imports[hex(address)] = dict(module=module,symbol=symbol)
                else:
                    virtual.append(dict(address=hex(row['address']),instruction=instruction))
        direct[span['name']] = calls
        unresolved[span['name']] = virtual
    assert direct['clip-change thunk'] == ['0x101afc40']
    # Register-held calls include imported QString construction and GUI setters;
    # preserve them as unresolved here rather than claiming all are pure getters.
    for address in (0x10a7b8fc,0x10a7c928,0x10a7cd64,0x10a7bc30,0x10a7bac0):
        module,symbol = imported_symbol(image,address)
        assert module in ('QtCore4.dll','QtGui4.dll')
        imports[hex(address)] = dict(module=module,symbol=symbol)
    data = BSP.read_bytes()
    assert data[:4] == b'VBSP'
    start,size = struct.unpack_from('<2I',data,8)
    assert start+size <= len(data)
    entities = [dict(re.findall(rb'"([^"]+)"\s*"([^"]*)"',entry))
                for entry in re.findall(rb'\{[^{}]+\}',data[start:start+size])]
    spawns = [{key.decode():value.decode() for key,value in entity.items()}
              for entity in entities if entity.get(b'classname') in (b'info_player_start',b'info_player_teamspawn')]
    assert Counter(row['classname'] for row in spawns) == {'info_player_start':3,'info_player_teamspawn':1}
    assert {str(path):sha(path) for path in (IFM,BSP)} == before
    receipt = dict(status='VERIFIED STATIC BOUNDARY; RUNTIME EFFECTS PARTIAL',original_hashes=before,
        direct_call_targets=direct,verified_imports=imports,unresolved_register_calls=unresolved,
        benchmark_spawns=spawns,verified_spans=[e['name'] for e in evidence],
        findings=['The installed clip-change thunk only directly calls the common viewport information refresh.',
                  'The two refresh bodies have no direct call to the known camera-to-player snap helper 10276790.',
                  'Resolved import slots belong to QtCore4/QtGui4 and include text, display and widget updates.',
                  'Benchmark spawn entities are present; runtime spawn selection and player lifetime are not established.'],
        limits=['Static installed-byte verification only; no refresh function is executed by this audit.',
                'Register-held/virtual calls and transitive callees remain listed, not assumed harmless.',
                'Qt signal delivery, getter side effects and live game-player behavior remain unverified.',
                'Spawn metadata is not evidence of a live player position or automatic door activation.',
                'No owned x64 editor change or visual compatibility improvement is claimed.'])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(verified_bytes=sum(e['bytes'] for e in evidence),qt_imports=len(imports),
                         spawn_entities=len(spawns),receipt=str(run/'results.json'))))


if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory',type=Path)
    main(parser.parse_args().directory.resolve())
