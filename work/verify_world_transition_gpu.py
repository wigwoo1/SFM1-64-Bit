"""Verify owned terrain-blend transport on GPU, without using SFM as an oracle."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
from datetime import datetime, timezone

from verify_session_preview_aspect import source_closure, bitmap, INCLUDE, VS, SESSION, CONTENT

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/world-transition-gpu-20261030'
CASES = [
    ('disabled', 255, 255, False, False, 1),
    ('negative', -127, -127, True, False, 1),
    ('zero', 0, 0, True, False, 1),
    ('quarter', 63.75, 63.75, True, False, 1),
    ('half', 127.5, 127.5, True, False, 1),
    ('full', 255, 255, True, False, 1),
    ('above', 510, 510, True, False, 1),
    ('gradient', 0, 255, True, False, 1),
    ('reverse', 255, 0, True, False, 1),
    ('clamped-gradient', -255, 510, True, False, 1),
    ('lit-half', 127.5, 127.5, True, True, 1),
    ('draw-alpha', 127.5, 127.5, True, False, .5),
]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def execute(args, cwd, log, env=None, success=True):
    process = subprocess.run([str(x) for x in args], cwd=cwd, env=env,
                             capture_output=True, text=True, timeout=600)
    text = process.stdout + process.stderr
    log.write_text(text, encoding='utf-8')
    if success:
        need(process.returncode == 0, f'{log}: exit {process.returncode}: {text[-2000:]}')
    return process.returncode, text


def amd64(path):
    data = path.read_bytes()
    pe = struct.unpack_from('<I', data, 60)[0]
    need(struct.unpack_from('<H', data, pe + 4)[0] == 0x8664 and
         struct.unpack_from('<H', data, pe + 24)[0] == 0x20b, 'AMD64 PE32+: ' + str(path))


def check_pixels(directory):
    results = {}
    for name, left, right, transition, lighting, alpha in CASES:
        path = directory / (name + '.rgba')
        data = path.read_bytes()
        need(len(data) == 64 * 32 * 4, name + ' image size')
        max_error = 0
        # Independent analytic reference: two known constant textures over a full quad.
        # Include every pixel, both triangles, and out-of-range blend weights.
        for y in range(32):
            for x in range(64):
                raw_weight = left + (right - left) * ((x + .5) / 64)
                weight = min(1, max(0, raw_weight / 255)) if transition else 0
                rgb = [a + (b - a) * weight for a, b in zip((32, 96, 160), (224, 160, 64))]
                if lighting:
                    rgb = [value * light for value, light in zip(rgb, (.25, .5, .75))]
                expected = [round(v) for v in rgb] + [round(255 * alpha)]
                actual = data[(y * 64 + x) * 4:(y * 64 + x + 1) * 4]
                error = max(abs(a - b) for a, b in zip(actual, expected))
                need(error <= 1, f'{name} pixel ({x},{y}): {list(actual)} expected {expected}')
                max_error = max(max_error, error)
        bitmap(path, 64, 32, directory / (name + '.bmp'))
        results[name] = dict(pixels_checked=2048, maximum_channel_error=max_error, sha256=sha(path))
    return results


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    (OUT / 'active-run.txt').write_text(str(run) + '\n', encoding='utf-8')
    sources = source_closure()
    def visit(path):
        path = path.resolve()
        if str(path) in sources:
            return
        sources[str(path)] = sha(path)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"', path.read_text(encoding='utf-8', errors='replace'), re.M):
            child = path.parent / include
            if child.is_file():
                visit(child)
    visit(Path(__file__))
    visit(ROOT / 'work/probe_world_transition_gpu.cpp')
    canonical = ROOT / 'outputs/sfm64-editor-x64/sfm64_session_ui.exe'
    protected = {str(p): sha(p) for p in [canonical, *sorted((canonical.parent / 'shaders').glob('*.spv'))]}
    originals = {str(p): sha(p) for p in (SESSION, CONTENT / 'gameinfo.txt') if p.is_file()}
    build = run / 'build'
    execute(['cmd.exe', '/d', '/c', ROOT / 'work/build_sfm64_editor_x64.cmd', build], ROOT, run / 'editor-build-driver.log')
    exe = build / 'sfm64_session_ui.exe'
    amd64(exe)
    print('Complete AMD64 editor and all 15 owned shaders built.', flush=True)
    probe = run / 'probe_world_transition_gpu.exe'
    batch = run / 'build-probe.cmd'
    batch.write_text('\n'.join([
        '@echo off', f'cd /d "{ROOT}"', f'call "{VS}" >nul',
        'if errorlevel 1 exit /b 1',
        f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 /I"{INCLUDE}" work\\probe_world_transition_gpu.cpp /Fo:"{run / "probe.obj"}" /Fe:"{probe}"',
        'exit /b %errorlevel%', '']), encoding='utf-8')
    execute(['cmd.exe', '/d', '/c', batch], ROOT, run / 'probe-build.log')
    amd64(probe)
    pixels = run / 'controlled-pixels'
    pixels.mkdir()
    vertex = build / 'shaders/sfm64-diagnostic-model.spv'
    fragment = build / 'shaders/sfm64-material-alpha-candidate.spv'
    execute([probe, vertex, fragment, pixels], run, run / 'probe.log')
    pixel_results = check_pixels(pixels)
    print('All 24,576 controlled pixels passed analytic RGBA comparison.', flush=True)
    stale = ROOT / 'build/session-core/sfm64-diagnostic-model.spv'
    expected_stale = 'd10f79efe0cdd42af798a152152001052acbf0ecc6cc57dc7ea6f5f0988ab29c'
    negative = dict(status='UNVERIFIED', reason='historical stale shader unavailable or changed')
    if stale.is_file() and sha(stale) == expected_stale:
        rejected = run / 'rejected-stale-shader'
        rejected.mkdir()
        code, text = execute([probe, stale, fragment, rejected], run, run / 'stale-shader.log', success=False)
        need(code != 0 and 'vertex shader lacks displacement alpha interface' in text and not list(rejected.iterdir()),
             'stale shader must be refused before Vulkan initialization')
        negative = dict(status='VERIFIED', shader_sha256=sha(stale), exit_code=code,
                        scope='probe preflight refuses the real historical shader; no undefined GPU draw')
    captures = {}
    benchmark = 'UNVERIFIED: benchmark session or content unavailable'
    if len(originals) == 2:
        session = run / 'input-copy.dmx'
        shutil.copyfile(SESSION, session)
        clean = {k: v for k, v in os.environ.items() if not k.startswith('SFM64_')}
        for name, app, options in [
            ('baseline', canonical, {}), ('candidate', exe, {}),
            ('baseline-two-texture', canonical, {'SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST': '1'}),
            ('candidate-two-texture', exe, {'SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST': '1'})]:
            directory = run / name
            directory.mkdir()
            env = dict(clean, SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',
                       SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory), **options)
            _, text = execute([app, '--control-test', '--session', session, '--content-directory', CONTENT,
                               '--start-frame', '119'], run, directory / 'run.log', env)
            need('model_gpu_presentation_passed=1 frames=5' in text and
                 'gpu_readback_copies=5 capture_frames=1' in text and 'scene_output_extent=512x288' in text,
                 name + ' complete owned-editor GPU sequence')
            captures[name] = {str(n): sha(directory / f'vulkan-retained-frame-{n}.rgba') for n in (0,119,120)}
            need(all((directory / f'vulkan-retained-frame-{n}.rgba').stat().st_size == 512 * 288 * 4 for n in (0,119,120)),
                 name + ' complete capture sizes')
            bitmap(directory / 'vulkan-retained-frame-120.rgba', 512, 288, directory / 'frame-120.bmp')
            print(name + ' five-frame owned-editor run passed.', flush=True)
        need(captures['baseline'] == captures['candidate'] and
             captures['baseline-two-texture'] == captures['candidate-two-texture'], 'exact existing benchmark pixels preserved')
        need(sha(session) == originals[str(SESSION)], 'disposable source copy unchanged')
        benchmark = 'VERIFIED: 20 GPU frames; retained frames 0/119/120 byte-identical in both modes'
    need(all(sha(p) == h for p, h in originals.items()), 'original inputs preserved')
    need(all(sha(p) == h for p, h in protected.items()), 'canonical published editor and shaders preserved')
    need(all(sha(p) == h for p, h in sources.items()), 'source closure unchanged during test')
    warnings = {name: re.findall(r'warning (C\d+): ([^\r\n]+)', path.read_text(errors='replace'))
                for name, path in [('editor', build / 'build.log'), ('probe', run / 'probe-build.log')]}
    result = dict(status='VERIFIED' if captures and negative['status'] == 'VERIFIED' else 'PARTIALLY VERIFIED',
        time_utc=datetime.now(timezone.utc).isoformat(),
        scope='owned shader blend transport and shared vertex layout; native Source material/lighting parity UNVERIFIED',
        editor=str(exe), editor_sha256=sha(exe), probe=str(probe), probe_sha256=sha(probe),
        vertex_shader_sha256=sha(vertex), fragment_shader_sha256=sha(fragment),
        pixels=pixel_results, stale_shader_negative_control=negative, benchmark=benchmark, captures=captures,
        compiler_warnings=warnings, source_sha256=sources, protected_original_inputs=originals,
        protected_published_runtime=protected)
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: result[key] for key in ('status','scope','editor','probe','benchmark')}, indent=2), flush=True)


if __name__ == '__main__':
    main()
