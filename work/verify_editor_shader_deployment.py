"""Fresh full build and extracted-package GPU regression; original SFM read only."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/editor-shader-deployment-20261030'
sys.path.insert(0, str(ROOT / 'publication'))
from editor_runtime_package import shader_manifest, owned_shader_payload, write_editor_package
from verify_session_preview_aspect import source_closure, bitmap


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def execute(args, cwd, log, env=None, success=True):
    process = subprocess.run([str(x) for x in args], cwd=cwd, env=env,
                             capture_output=True, text=True, timeout=180)
    text = process.stdout + process.stderr
    log.write_text(text, encoding='utf-8')
    if success:
        need(process.returncode == 0, f'{log}: exit {process.returncode}: {text[-1500:]}')
    return process.returncode, text


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    sources = source_closure()
    for path in (Path(__file__), ROOT / 'publication/editor_runtime_package.py',
                 ROOT / 'publication/prepare_public_snapshot.py', ROOT / 'work/test_editor_shader_resources.cpp'):
        sources[str(path)] = sha(path)
    rows = shader_manifest(ROOT)
    references = {name for path in sources for name in re.findall(
        r'"(?:build/session-core/)?(sfm64-[a-z0-9-]+\.spv)"', Path(path).read_text(errors='replace'))}
    need(references == {stem + '.spv' for _, stem in rows}, 'complete runtime shader manifest coverage')
    original = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx')
    content = original.parents[4] / 'usermod'
    originals = {str(p): sha(p) for p in (original, content / 'gameinfo.txt')}
    session = run / 'input-copy.dmx'
    shutil.copyfile(original, session)
    prior = json.loads((ROOT / 'outputs/session-preview-aspect-20261030/latest-verification.json').read_text())
    baseline = Path(json.loads(Path(prior['receipt']).read_text())['executable'])
    baseline_sha = sha(baseline)
    canonical = ROOT / 'outputs/sfm64-editor-x64/sfm64_session_ui.exe'
    canonical_sha = sha(canonical)
    build = run / 'build'
    execute(['cmd.exe', '/d', '/c', ROOT / 'work/build_sfm64_editor_x64.cmd', build],
            ROOT, run / 'build-driver.log')
    exe = build / 'sfm64_session_ui.exe'
    data = exe.read_bytes()
    pe = struct.unpack_from('<I', data, 60)[0]
    need(struct.unpack_from('<H', data, pe + 4)[0] == 0x8664 and
         struct.unpack_from('<H', data, pe + 24)[0] == 0x20b, 'native AMD64 PE32+')
    print('Full editor and owned shaders built.', flush=True)
    foreign = run / 'unrelated working directory'
    foreign.mkdir()
    unit = run / 'test_editor_shader_resources.exe'
    batch = run / 'build-path-test.cmd'
    batch.write_text('\n'.join([
        '@echo off', f'cd /d "{ROOT}"',
        r'call "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul',
        'if errorlevel 1 exit /b 1',
        f'cl /nologo /std:c++17 /EHsc /W4 /O2 work\\test_editor_shader_resources.cpp /Fo:"{run / "path-test.obj"}" /Fe:"{unit}"',
        'exit /b %errorlevel%', '']))
    execute(['cmd.exe', '/d', '/c', batch], ROOT, run / 'path-test-build.log')
    execute([unit, foreign], ROOT, run / 'path-test.log')
    shaders = owned_shader_payload(ROOT, build)
    metadata = dict(executable_sha256=sha(exe), source_sha256={str(Path(p).relative_to(ROOT)): h for p, h in sources.items()},
                    runtime_shader_sha256={name: hashlib.sha256(b).hexdigest() for name, b in shaders.items()})
    packages = [run / 'editor-runtime.zip', run / 'editor-runtime-repeat.zip']
    for package in packages:
        package_files = write_editor_package(package, data, shaders, json.dumps(metadata).encode(),
            b'Experimental owned editor. Keep shaders/ beside the executable. Supply your own SFM content.\n',
            b'Owned HLSL shaders; no original Valve binaries or assets are included.\n')
    need(sha(packages[0]) == sha(packages[1]), 'deterministic runtime package')
    extracted = run / 'extracted build with spaces Ω'
    extracted.mkdir()
    with zipfile.ZipFile(packages[0]) as archive:
        need(len(archive.namelist()) == len(shaders) + 5, 'strict runtime ZIP contents')
        need(not any(Path(n).suffix in {'.dmx', '.dll', '.hlsl', '.obj', '.log'} for n in archive.namelist()),
             'forbidden runtime package content')
        archive.extractall(extracted)
    for name, expected in package_files.items():
        need(sha(extracted / name) == expected, 'extracted file identity: ' + name)
    clean = {k: v for k, v in os.environ.items() if not k.startswith('SFM64_')}
    frames = {}
    for name, app, cwd, options in [
        ('baseline', baseline, ROOT, {}),
        ('extracted-foreign', extracted / 'sfm64_session_ui.exe', foreign, {}),
        ('extracted-workspace', extracted / 'sfm64_session_ui.exe', ROOT, {}),
        ('baseline-two-texture', baseline, ROOT, {'SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST': '1'}),
        ('extracted-two-texture', extracted / 'sfm64_session_ui.exe', foreign, {'SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST': '1'})]:
        directory = run / name
        directory.mkdir()
        env = dict(clean, SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',
                   SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory), **options)
        _, text = execute([app, '--control-test', '--session', session, '--content-directory', content,
                           '--start-frame', '119'], cwd, directory / 'run.log', env)
        need('model_gpu_presentation_passed=1 frames=5' in text and
             'gpu_readback_copies=5 capture_frames=1' in text and
             'scene_output_extent=512x288' in text, name + ' complete GPU sequence')
        frames[name] = {str(n): sha(directory / f'vulkan-retained-frame-{n}.rgba') for n in (0, 119, 120)}
        need(all((directory / f'vulkan-retained-frame-{n}.rgba').stat().st_size == 512 * 288 * 4 for n in (0,119,120)),
             name + ' capture size')
        bitmap(directory / 'vulkan-retained-frame-120.rgba', 512, 288, directory / 'frame-120.bmp')
        print(name + ' GPU capture passed.', flush=True)
    need(frames['baseline'] == frames['extracted-foreign'] == frames['extracted-workspace'],
         'default render byte parity and CWD independence')
    need(frames['baseline-two-texture'] == frames['extracted-two-texture'], 'two-texture render byte parity')
    _, default_log = execute([extracted / 'sfm64_session_ui.exe', '--control-test', '--session', session,
                             '--content-directory', content, '--start-frame', '119'],
                            foreign, run / 'default-launch.log', clean)
    need('model_gpu_presentation_passed=1 frames=5' in default_log and
         'gpu_readback_copies=0 capture_frames=0' in default_log,
         'normal launch requires no diagnostic environment or capture directory')
    need(not (foreign / 'outputs').exists() and not list(foreign.glob('*.rgba')),
         'normal preview must not create diagnostic frame output')
    missing = run / 'missing resources'
    missing.mkdir()
    shutil.copyfile(exe, missing / exe.name)
    code, text = execute([missing / exe.name, '--control-test', '--session', session,
                          '--content-directory', content, '--start-frame', '119'],
                         ROOT, run / 'missing-resources.log', dict(clean, SFM64_DIAGNOSTIC_CAPTURE_FRAMES='0'), success=False)
    need(code != 0 and 'pipeline shader missing' in text, 'missing resources fail without workspace fallback')
    need(all(sha(p) == h for p, h in originals.items()), 'original inputs unchanged')
    need(all(sha(p) == h for p, h in sources.items()), 'sources unchanged during verification')
    need(sha(baseline) == baseline_sha and sha(canonical) == canonical_sha, 'prior editors preserved')
    shader_differences = [stem for _, stem in rows if sha(build / 'shaders' / (stem + '.spv')) !=
                          sha(ROOT / 'build/session-core' / (stem + '.spv'))]
    warnings = re.findall(r'warning (C\d+): ([^\r\n]+)', (build / 'build.log').read_text(errors='replace'))
    result = dict(status='VERIFIED', scope='owned shader deployment and package runtime, not SFM parity',
        executable=str(exe), executable_sha256=sha(exe), package=str(packages[0]), package_sha256=sha(packages[0]),
        shader_count=len(shaders), shader_source_manifest=str(ROOT / 'work/editor_shader_manifest.txt'),
        runtime_file_sha256=package_files, shader_binary_differences_from_workspace=shader_differences,
        gpu_frames=30, capture_sha256=frames, compiler_warnings=warnings, source_sha256=sources,
        protected_original_inputs=originals, canonical_editor_preserved=True,
        missing_resources='rejected even from workspace CWD', working_directories='foreign and workspace',
        default_launch='passed without diagnostic environment or capture directory',
        optional_shader_scope='All compiled/packaged; default and two-texture paths exercised on GPU')
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(receipt=str(receipt),receipt_sha256=sha(receipt)),indent=2)+'\n')
    print(json.dumps({k: result[k] for k in ('status','executable','package','shader_count','gpu_frames')},indent=2),flush=True)


if __name__ == '__main__':
    main()
