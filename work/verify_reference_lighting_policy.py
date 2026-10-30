"""Check the user's explicit reference profile against existing owned policies.

No native SFM process, DLL execution, GPU calls, original writes or editor changes.
"""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile

from verify_session_preview_aspect import ROOT, VS, source_closure
from verify_ifm_draw_packet import check_ranges

OUT = ROOT / 'outputs/default-lighting-handoff-20261030'
GAME = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def need(value, why):
    if not value:
        raise RuntimeError(why)


def main():
    profile_path = OUT / 'reference-profile.json'
    profile = json.loads(profile_path.read_text(encoding='utf-8'))
    controls = profile['user_reported_console_values']
    need(profile['dx_support_level']['value'] == 95 and not profile['dx_support_level']['native_verified'], 'assumed DX95 provenance')
    need(controls == dict(mat_phong=1, mat_hdr_level=3, r_radiosity=4, r_lightcachecenter=1, sfm_userootbonefororigin=1), 'reported controls')
    latest = json.loads((OUT / 'latest-verification.json').read_text())
    prior_path = Path(latest['receipt'])
    need(sha(prior_path) == latest['receipt_sha256'], 'default handoff receipt identity')
    prior = json.loads(prior_path.read_text())
    for group in ('source_sha256', 'protected_input_sha256', 'protected_runtime_sha256'):
        need(all(sha(p) == h for p, h in prior[group].items()), 'handoff inputs/source/runtime remain current: ' + group)
    run = Path(tempfile.mkdtemp(prefix='reference-policy-', dir=OUT))
    protected = {str(profile_path): sha(profile_path), str(prior_path): sha(prior_path),
                 **prior['protected_input_sha256'], **prior['protected_runtime_sha256']}
    pins = [('engine.dll', 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'),
            ('shaderapidx9.dll', '07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'),
            ('stdshader_dx9.dll', 'fb09e2ddc8a99aff4ef31e3fddfdb1ca84d89379336abdab4e76e94adfa7e6f8')]
    for name, expected in pins:
        path = GAME / 'bin' / name
        need(sha(path) == expected, 'pinned reference binary changed: ' + name)
        protected[str(path)] = expected
    ranges = {}
    for name, disassembly, spans in [
        ('engine.dll', 'engine-disassembly.txt', [('map_hdr_request', 0x100e34a0, 0x100e3556)]),
        ('shaderapidx9.dll', 'shaderapidx9-disassembly.txt', [('current_hdr_query', 0x10003940, 0x1000398b), ('dx_level_getter', 0x100034f0, 0x100034f7)]),
        ('stdshader_dx9.dll', 'stdshader_dx9-disassembly.txt', [('typed_predicate', 0x10054420, 0x10054487), ('draw_hardware_gate', 0x1005a820, 0x1005a86f)])]:
        _, checked = check_ranges(GAME / 'bin' / name, disassembly, spans)
        # Keep addresses/counts in the receipt; do not copy original instruction arrays.
        ranges[name] = [{k: v for k, v in row.items() if k != 'instructions'} for row in checked]
    sources = source_closure()
    def visit(path):
        path = path.resolve()
        if str(path) in sources:
            return
        sources[str(path)] = sha(path)
        for name in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', path.read_text(encoding='utf-8', errors='replace'), re.M):
            child = path.parent / name
            if child.is_file():
                visit(child)
    for name in ['audit_reference_lighting_policy.cpp', 'test_shader_skin_material_predicate.cpp', Path(__file__).name]:
        visit(ROOT / 'work' / name)
    programs = ['audit_reference_lighting_policy', 'test_shader_skin_material_predicate']
    commands = ['@echo off', f'cd /d "{ROOT}"', f'call "{VS}" >nul', 'if errorlevel 1 exit /b 1']
    for name in programs:
        commands += [f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{name}.cpp /Fo:"{run / (name + ".obj")}" /Fe:"{run / (name + ".exe")}"', 'if errorlevel 1 exit /b 1']
    commands += ['exit /b 0', '']
    batch = run / 'build.cmd'
    batch.write_text('\n'.join(commands), encoding='utf-8')
    def execute(args, log):
        p = subprocess.run([str(x) for x in args], cwd=ROOT,
                           env={k: v for k, v in os.environ.items() if not k.startswith('SFM64_')},
                           capture_output=True, text=True, timeout=90)
        log.write_text(p.stdout + p.stderr, encoding='utf-8')
        need(p.returncode == 0, f'{log}: {p.returncode}: {(p.stdout + p.stderr)[-1500:]}')
        return p.stdout
    execute(['cmd.exe', '/d', '/c', batch], run / 'build.log')
    binaries = {}
    for name in programs:
        path = run / (name + '.exe')
        raw = path.read_bytes()
        pe = struct.unpack_from('<I', raw, 60)[0]
        need(struct.unpack_from('<H', raw, pe + 4)[0] == 0x8664 and struct.unpack_from('<H', raw, pe + 24)[0] == 0x20b, 'AMD64 PE32+: ' + name)
        binaries[name] = dict(path=str(path), sha256=sha(path))
    audit = execute([binaries[programs[0]]['path'], GAME / 'tf_movies/maps/mtt_soldier.bsp', 95, controls['mat_hdr_level'], controls['mat_phong']], run / 'scenarios.jsonl')
    rows = [json.loads(line) for line in audit.splitlines()]
    need(len(rows) == 37 and rows[-1]['conditional_scenarios'] == 36, 'all explicit scenarios')
    for row in rows[:-1]:
        need(row['phong_draw_gate'] == int(row['dx'] >= 92 and row['mat_phong'] != 0), 'conditional Phong admission')
        need(row['ambient_storage'] == ('hdr' if row['fixture_hardware_hdr_type'] else 'ldr'), 'conditional ambient dataset')
    test = execute([binaries[programs[1]]['path']], run / 'predicate-regression.log')
    need('PASS checks=273 typed_effective_inputs_only=1' in test, 'existing typed predicate regression')
    need(all(sha(p) == h for p, h in sources.items()), 'source unchanged during test')
    need(all(sha(p) == h for p, h in protected.items()), 'protected evidence/original inputs/published runtime preserved')
    warnings = re.findall(r'warning (C\d+): ([^\r\n]+)', (run / 'build.log').read_text(errors='replace'))
    result = dict(status='VERIFIED', scope='conditional policy checks only; actual native configuration PARTIALLY VERIFIED',
        time_utc=datetime.now(timezone.utc).isoformat(), profile=profile, profile_sha256=sha(profile_path),
        scenarios=36, typed_predicate_regression_checks=273, executables=binaries, compiler_warnings=warnings,
        checked_native_instruction_bytes=sum(row['bytes'] for rows_ in ranges.values() for row in rows_),
        authenticated_ranges=ranges, source_sha256=sources, protected_sha256=protected,
        earlier_default_handoff_receipt=str(prior_path), earlier_default_handoff_receipt_sha256=sha(prior_path),
        editor_changed=False, gpu_calls=0, native_code_executed=False,
        limitations=['DX95 is user-authorized assumed input, not a measured runtime capability.',
                     'Typed material inputs, hardware HDR types, GL emulation, object aliasing and base BSP header-only lengths are explicit fixtures.',
                     'No native material initialization, HDR notification lifecycle, active shader combination, model origin/cache or image parity is established.'])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    (OUT / 'latest-reference-policy.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: result[k] for k in ['status', 'scenarios', 'typed_predicate_regression_checks', 'checked_native_instruction_bytes', 'compiler_warnings', 'executables']}, indent=2))


if __name__ == '__main__':
    main()
