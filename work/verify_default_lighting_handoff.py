"""Fresh CPU observation of actual default draw preparation; no native process/GPU."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

from verify_session_preview_aspect import source_closure, ROOT, INCLUDE, VS, SESSION, CONTENT

OUT = ROOT / 'outputs/default-lighting-handoff-20261030'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def need(value, reason):
    if not value:
        raise RuntimeError(reason)


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    (OUT / 'active-run.txt').write_text(str(run) + '\n', encoding='utf-8')
    protected = {str(p): sha(p) for p in [SESSION, CONTENT / 'gameinfo.txt', CONTENT.parent / 'tf_movies/maps/mtt_soldier.bsp']}
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
    visit(ROOT / 'work/audit_default_lighting_handoff.cpp')
    visit(Path(__file__))
    canonical = ROOT / 'outputs/sfm64-editor-x64'
    runtime = {str(p): sha(p) for p in [canonical / 'sfm64_session_ui.exe', *sorted((canonical / 'shaders').glob('*.spv'))]}
    session = run / 'input-copy.dmx'
    shutil.copyfile(SESSION, session)
    exe = run / 'audit_default_lighting_handoff_x64.exe'
    batch = run / 'build-audit.cmd'
    batch.write_text('\n'.join([
        '@echo off', f'cd /d "{ROOT}"', f'call "{VS}" >nul',
        'if errorlevel 1 exit /b 1',
        f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\audit_default_lighting_handoff.cpp /Fo:"{run / "audit.obj"}" /Fe:"{exe}"',
        'exit /b %errorlevel%', '']), encoding='utf-8')
    def execute(args, log):
        p = subprocess.run([str(x) for x in args], cwd=ROOT,
                           env={k: v for k, v in os.environ.items() if not k.startswith('SFM64_')},
                           capture_output=True, text=True, timeout=180)
        log.write_text(p.stdout + p.stderr, encoding='utf-8')
        need(p.returncode == 0, f'{log}: exit {p.returncode}: {(p.stdout + p.stderr)[-1600:]}')
        return p.stdout
    execute(['cmd.exe', '/d', '/c', batch], run / 'build.log')
    raw = exe.read_bytes()
    pe = struct.unpack_from('<I', raw, 60)[0]
    need(struct.unpack_from('<H', raw, pe + 4)[0] == 0x8664 and struct.unpack_from('<H', raw, pe + 24)[0] == 0x20b,
         'native AMD64 PE32+')
    output = execute([exe, session, CONTENT / 'gameinfo.txt'], run / 'audit.jsonl')
    rows = [json.loads(line) for line in output.splitlines()]
    frames = [r for r in rows if r['kind'] == 'frame']
    materials = [r for r in rows if r['kind'] == 'material']
    draws = [r for r in rows if r['kind'] == 'draw']
    need(len(frames) == 3 and rows[-1]['status'] == 'VERIFIED', 'three complete evaluated frames')
    per_frame = []
    for frame in frames:
        tick = frame['tick']
        ms = [r for r in materials if r['tick'] == tick]
        ds = [r for r in draws if r['tick'] == tick]
        need(len(ms) == frame['material_slots'] and len(ds) == frame['draws'], 'complete recorded material/draw coverage')
        slots = {m['slot']: m for m in ms}
        counts = {}
        for m in ms:
            counts[m['shader']] = counts.get(m['shader'], 0) + 1
        actors = [m for m in ms if m['shader'] == 'vertexlitgeneric']
        actor_draws = [d for d in ds if slots[d['slot']]['shader'] == 'vertexlitgeneric']
        need(len(actors) == 3 and actor_draws and all(d['vertex_lighting'] == 0 for d in actor_draws), 'actual actor lighting flag absence')
        need(all(m['declared'].get('$phong') == '1' for m in actors), 'authored Phong declarations')
        need(frame['projected_lights'] == 0 and all(d['fragment_bytes'] == 48 and d['fragment_offset'] == 96 for d in ds),
             'no projected lights and default fragment payload')
        per_frame.append(dict(tick=tick, shader_counts=counts, actor_draws=len(actor_draws),
                              actor_slots=[m['slot'] for m in actors], frame=frame))
    vertex = (ROOT / 'work/sfm64-diagnostic-model.hlsl').read_text(encoding='utf-8')
    fragment = (ROOT / 'work/sfm64-material-alpha-candidate.hlsl').read_text(encoding='utf-8')
    bindings = [(int(n), name) for n, name in re.findall(
        r'\[\[vk::binding\((\d+),\s*0\)\]\]\s*(?:Texture2D<float4>|SamplerState)\s+(\w+)', fragment)]
    need(bindings == [(0, 'baseTexture'), (1, 'baseSampler'), (3, 'secondBaseTexture')], 'default shader resources')
    need('output.vertexLight = input.normal;' in vertex and 'if (material.vertexLighting != 0) sampled.rgb *= vertexLight;' in fragment,
         'normal/light channel and optional modulation dataflow')
    dependency_names = {Path(p).name for p in source_closure()}
    absent_helpers = [name for name in ['scene_model_ambient_records_candidate.h', 'generic_ambient_irradiance_candidate.h',
        'session_ifm_lighting_origin_candidate.h', 'source_bsp_ambient_selection_candidate.h',
        'vertexlit_concrete_init_candidate.h', 'shader_skin_material_predicate_candidate.h'] if name not in dependency_names]
    need(sha(session) == protected[str(SESSION)] and all(sha(p) == h for p, h in protected.items()), 'original and disposable inputs unchanged')
    need(all(sha(p) == h for p, h in runtime.items()), 'published runtime unchanged')
    need(all(sha(p) == h for p, h in sources.items()), 'source closure unchanged during audit')
    warnings = re.findall(r'warning (C\d+): ([^\r\n]+)', (run / 'build.log').read_text(errors='replace'))
    result = dict(status='VERIFIED', scope='CPU default-input/command handoff, not native rendering or GPU parity',
        time_utc=datetime.now(timezone.utc).isoformat(), executable=str(exe), executable_sha256=sha(exe),
        frames=per_frame, materials_observed=len(materials), draws_observed=len(draws),
        default_fragment_bindings=bindings, helpers_absent_from_editor_include_closure=absent_helpers,
        gpu_calls=0, compiler_warnings=warnings, audit_sha256=sha(run / 'audit.jsonl'),
        source_sha256=sources, protected_input_sha256=protected, protected_runtime_sha256=runtime,
        limitations=['Raw authored declarations are not effective typed native material values.',
                     'Fake Vulkan handles and observer callbacks record CPU commands only; no GPU APIs were called.',
                     'Native runtime controls, active shader variants, original model lighting and visual parity remain UNVERIFIED.'])
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: result[k] for k in ['status','executable','materials_observed','draws_observed','gpu_calls','helpers_absent_from_editor_include_closure']}, indent=2))


if __name__ == '__main__':
    main()
