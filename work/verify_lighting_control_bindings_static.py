"""Offline-only control bindings. Never launch, attach to, or modify original SFM.

Disassembly is local evidence, checked against pinned on-disk DLLs. The receipt
records addresses/meaning, not original machine code. Registered defaults are
NOT observations of live values or permission to change renderer defaults.
"""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import tempfile

from verify_ifm_draw_packet import check_ranges

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/native-lighting-reference-20261024'
GAME = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
PINNED = {
    'ifm': (GAME / 'bin/tools/ifm.dll', '8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4'),
    'engine': (GAME / 'bin/engine.dll', 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'),
}
RANGES = {
    'ifm': [
        ('root_registration', 0x10A69120, 0x10A6913B),
        ('draw_registration', 0x10A69240, 0x10A69256),
        ('short_constructor', 0x1097D0E0, 0x1097D147),
        ('help_constructor', 0x1097D150, 0x1097D1BA),
        ('initializer_self_link', 0x1097CE00, 0x1097CE0C),
        ('child_root_gate', 0x1023BD04, 0x1023BD13),
        ('first_bone_root_gate', 0x1023F5A8, 0x1023F5B8),
        ('wrapper_root_gate', 0x10240559, 0x10240565),
        ('packet_draw_gate', 0x10243B44, 0x10243B8C),
    ],
    'engine': [
        ('radiosity_registration', 0x1030F710, 0x1030F72E),
        ('center_registration', 0x1030F860, 0x1030F879),
        ('short_constructor', 0x1024FC70, 0x1024FCD7),
        ('help_constructor', 0x1024FCE0, 0x1024FD4A),
        ('initializer_self_link', 0x1024F990, 0x1024F99C),
        ('radiosity_selector', 0x10193207, 0x10193220),
        ('center_gate', 0x1019794A, 0x10197968),
    ],
}
CONTROLS = [
    dict(module='ifm', name='sfm_userootbonefororigin', default='1',
         name_va=0x10ADBAE0, default_va=0x10A95A68, object_va=0x10F31758,
         registration='root_registration', constructor=0x1097D150,
         consumers=[0x1023BD04, 0x1023F5A8, 0x10240559],
         meaning='Root-bone-origin policy in child, first-bone and wrapper bounds paths; distinct from draw enable.'),
    dict(module='ifm', name='sfm_drawpuppetmodels', default='1',
         name_va=0x10ADBC28, default_va=0x10A95A68, object_va=0x10F31968,
         registration='draw_registration', constructor=0x1097D0E0,
         consumers=[0x10243B44], meaning='Optional model-interface slot16 draw dispatch; packet construction precedes the gate.'),
    dict(module='engine', name='r_radiosity', default='4',
         name_va=0x1035BDEC, default_va=0x1031EC94, object_va=0x1064A818,
         registration='radiosity_registration', constructor=0x1024FCE0,
         consumers=[0x10193207], meaning='Ambient calculation selector; registered value4 is not a measured live setting.'),
    dict(module='engine', name='r_lightcachecenter', default='1',
         name_va=0x1035BEE4, default_va=0x1031DA24, object_va=0x10662618,
         registration='center_registration', constructor=0x1024FC70,
         consumers=[0x1019794A], meaning='Cache-miss centering policy; nonzero calls101947B0, zero follows original-point branch.'),
]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='control-bindings-', dir=OUT))
    checks = []

    def require(value, label):
        if not value:
            raise RuntimeError(label)
        checks.append(label)

    tracked = [ROOT / p for p in [
        'work/probe_vulkan_win32_surface_x64.cpp',
        'work/ifm_renderable_bounds_candidate.h',
        'work/ifm_model_draw_packet_candidate.h',
        'work/session_ifm_lighting_origin_candidate.h',
        'work/vulkan_model_draw_candidate.h',
        'work/win32_vulkan_playback_candidate.h',
        'work/vtf_texture_layout.h',
        'outputs/vtf-legacy-thumbnail-20261024/sfm64_session_ui_vtf_legacy.exe',
    ]]
    owned_before = {str(p.relative_to(ROOT)): sha(p) for p in tracked}
    originals = json.loads((OUT / 'launch-inputs.json').read_text())['original_inputs_sha256']
    original_now = {p: sha(p) for p in originals}
    images, evidence, rows = {}, {}, {}
    for module, (path, digest) in PINNED.items():
        require(sha(path) == digest, module + ' pinned DLL hash')
        images[module], evidence[module] = check_ranges(path, module + '-disassembly.txt', RANGES[module])
        rows[module] = {i['address']: i for r in evidence[module] for i in r['instructions']}

    def instruction(module, va, expected):
        actual = rows[module][va]['instruction']
        require(''.join(actual.lower().split()) == ''.join(expected.lower().split()),
                module + ':' + hex(va) + ' ' + expected)

    bindings = []
    for c in CONTROLS:
        module, image = c['module'], images[c['module']]
        require(image.string(c['name_va']) == c['name'], c['name'] + ' name')
        require(image.string(c['default_va']) == c['default'], c['name'] + ' registered default string')
        registration = next(r for r in evidence[module] if r['name'] == c['registration'])
        tail = registration['instructions'][-4:]
        expected = [f"push {c['default_va']:08X}h", f"push {c['name_va']:08X}h",
                    f"mov ecx,{c['object_va']:08X}h", f"call {c['constructor']:08X}"]
        for row, text in zip(tail, expected):
            instruction(module, row['address'], text)
        for va in c['consumers']:
            # x86 absolute pointer load, followed by integer read/compare at+30.
            inst = rows[module][va]['instruction']
            require(f"[{c['object_va'] + 0x1C:08X}h]".lower() in inst.lower(), c['name'] + ' consumer ' + hex(va))
        bindings.append({k: v for k, v in c.items() if k not in ('name_va', 'default_va')})
        bindings[-1].update(parent_pointer_slot=hex(c['object_va'] + 0x1C),
                            integer_field_offset='0x30', live_value='UNVERIFIED')

    for module, short_call, help_call, init, self_link in [
        ('ifm', 0x1097D13B, 0x1097D1AE, 0x1097CE00, 0x1097CE09),
        ('engine', 0x1024FCCB, 0x1024FD3E, 0x1024F990, 0x1024F999),
    ]:
        instruction(module, short_call, f'call {init:08X}')
        instruction(module, help_call, f'call {init:08X}')
        instruction(module, self_link, 'mov dword ptr [esi+1Ch],esi')
    for va, text in [
        (0x1023BD09, 'cmp dword ptr [eax+30h],0'),
        (0x1023BD0D, 'je 1023BDA4'),
        (0x1023F5AE, 'cmp dword ptr [ecx+30h],0'),
        (0x1023F5B2, 'jne 1023F685'),
        (0x1024055F, 'cmp dword ptr [ecx+30h],0'),
        (0x10240563, 'je 10240590'),
        (0x10243B73, 'cmp dword ptr [ecx+30h],edi'),
        (0x10243B76, 'je 10243B8C'),
        (0x10243B80, 'mov edx,dword ptr [edx+40h]'),
        (0x10243B8A, 'call edx'),
    ]:
        instruction('ifm', va, text)
    for va, text in [
        (0x1019320C, 'mov eax,dword ptr [eax+30h]'),
        (0x1019320F, 'dec eax'), (0x10193210, 'cmp eax,3'),
        (0x10193213, 'ja 101932B7'),
        (0x10193219, 'jmp dword ptr [eax*4+101932E8h]'),
        (0x1019794F, 'cmp dword ptr [eax+30h],0'),
        (0x10197953, 'je 1019799C'), (0x1019795E, 'call 101947B0'),
    ]:
        instruction('engine', va, text)
    require(CONTROLS[0]['object_va'] != CONTROLS[1]['object_va'], 'Root-origin and draw controls are distinct objects')
    require(images['ifm'].string(0x10ADBB00) ==
            'Use the root bone of a model as the origin for rendering, culling, shadows, etc.', 'Root-origin registered help')
    for module, (path, digest) in PINNED.items():
        require(sha(path) == digest, module + ' DLL unchanged after inspection')
    require(all(sha(p) == owned_before[str(p.relative_to(ROOT))] for p in tracked), 'Tracked owned editor sources and executable unchanged')
    require(all(sha(p) == digest for p, digest in original_now.items()), 'Original files unchanged during offline verification')
    changed_since_launch = [p for p in originals if originals[p] != original_now[p]]
    receipt = dict(
        status='PASS', scope='Static bindings verified; runtime lighting and full SFM parity UNVERIFIED',
        checked_at_utc=datetime.now(timezone.utc).isoformat(), checks=len(checks), check_labels=checks,
        bindings=bindings, pinned_dlls={k: dict(path=str(p), sha256=d) for k, (p, d) in PINNED.items()},
        ranges={k: [dict(name=r['name'], start=r['start'], end_exclusive=r['end_exclusive'], bytes=r['bytes'])
                    for r in es] for k, es in evidence.items()},
        static_instruction_bytes=sum(r['bytes'] for es in evidence.values() for r in es),
        screenshot=dict(path=str(OUT / 'source-sfm-frame120-user.png'), sha256=sha(OUT / 'source-sfm-frame120-user.png'),
                        origin='User supplied; visual reference only; not numerical lighting evidence'),
        original_sha256_at_verification=original_now, changed_since_prelaunch_snapshot=changed_since_launch,
        tracked_owned_files_sha256=owned_before, verifier_sha256=sha(__file__),
        limitations=[
            'No executable/DLL loading or original instruction execution; no process enumeration, memory reading, UI control or capture.',
            'Constructor/default registration and consumers checked against original disk bytes, not running ConVar state.',
            'Existing numerical bounds/cache candidates are not reverified or integrated by this task.',
            'No production renderer changes, compiler invocation, editor rebuild, GPU test or new rendering parity claim.',
            'Earlier custom reference launch failed twice; stopped. User reported SFM works with computer use off. Cause UNVERIFIED.',
        ])
    target = run / 'results.json'
    target.write_text(json.dumps(receipt, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(status=receipt['status'], checks=len(checks), static_instruction_bytes=receipt['static_instruction_bytes'],
                          changed_since_prelaunch_snapshot=changed_since_launch, receipt=str(target))))


if __name__ == '__main__':
    main()
