"""Build and exercise session preview sizing with disposable benchmark copies."""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/session-preview-aspect-20261030'
VS = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
DUMP = VS.parents[2] / 'Tools/MSVC/14.51.36231/bin/Hostx64/x64/dumpbin.exe'
SESSION = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\elements\sessions\mtt_soldier\mtt_soldier.dmx')
CONTENT = SESSION.parents[4] / 'usermod'
INCLUDE = ROOT / 'work/dependencies/vulkan-headers-6802bb4733b6/Vulkan-Headers-6802bb4733b63ed5efd3adb308a6c885ef180ea1/include'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def source_closure():
    paths = set()
    def visit(path):
        path = path.resolve()
        if path in paths:
            return
        paths.add(path)
        for include in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',
                                  path.read_text(errors='replace'), re.M):
            child = path.parent / include
            if child.is_file():
                visit(child)
    for name in ('probe_vulkan_win32_surface_x64.cpp', 'test_live_movie_output_extent_x64.cpp',
                 'build_sfm64_editor_x64.cmd', 'build_editor_shader_resources.cmd',
                 'editor_shader_manifest.txt', Path(__file__).name):
        visit(ROOT / 'work' / name)
    for line in (ROOT / 'work/editor_shader_manifest.txt').read_text().splitlines():
        if line.strip() and not line.startswith('#'):
            visit(ROOT / 'work' / (line.split()[1] + '.hlsl'))
    return {str(p): sha(p) for p in sorted(paths)}


def execute(args, log, env=None, timeout=180):
    result = subprocess.run([str(a) for a in args], cwd=ROOT, env=env,
                            capture_output=True, text=True, timeout=timeout)
    text = result.stdout + result.stderr
    log.write_text(text, encoding='utf-8')
    need(result.returncode == 0, f'{log.name} failed ({result.returncode}): {text[-2000:]}')
    return text


def bitmap(source, width, height, destination):
    rgba = source.read_bytes()
    need(len(rgba) == width * height * 4, 'capture byte count')
    bgra = bytearray(rgba)
    bgra[0::4], bgra[2::4] = rgba[2::4], rgba[0::4]
    header = struct.pack('<2sIHHI', b'BM', 54 + len(bgra), 0, 0, 54)
    header += struct.pack('<IiiHHIIiiII', 40, width, -height, 1, 32, 0,
                          len(bgra), 2835, 2835, 0, 0)
    destination.write_bytes(header + bgra)


def main():
    OUT.mkdir(exist_ok=True)
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    original_hashes = {str(p): sha(p) for p in (SESSION, CONTENT / 'gameinfo.txt')}
    sources = source_closure()
    baseline = ROOT / 'outputs/sfm64-editor-x64/sfm64_session_ui.exe'
    baseline_hash = sha(baseline)
    session = run / 'input-copy.dmx'
    shutil.copyfile(SESSION, session)
    test = run / 'test_live_movie_output_extent_x64.exe'
    editor = run / 'sfm64_session_ui.exe'
    (run / 'temp').mkdir()
    # Complete editor translation unit, libraries, and flags match the existing
    # work/build_sfm64_editor_x64.cmd; only output paths are isolated here.
    flags = '/nologo /std:c++17 /fp:strict /EHsc /W4 /O2'
    command = '\n'.join([
        '@echo off', 'setlocal', f'cd /d "{ROOT}"',
        f'call "{VS}" >nul', 'if errorlevel 1 exit /b 1',
        f'set "TEMP={run / "temp"}"', f'set "TMP={run / "temp"}"',
        f'cl {flags} /I"{INCLUDE}" work\\test_live_movie_output_extent_x64.cpp /Fo:"{run / "test.obj"}" /Fe:"{test}" >"{run / "test-build.log"}" 2>&1',
        'if errorlevel 1 exit /b 1',
        f'call "{ROOT / "work/build_sfm64_editor_x64.cmd"}" "{run}"',
        'exit /b %errorlevel%', ''])
    batch = run / 'build.cmd'
    batch.write_text(command, encoding='utf-8')
    execute(['cmd.exe', '/d', '/c', batch], run / 'build-driver.log')
    for name, exe in [('test', test), ('editor', editor)]:
        binary = exe.read_bytes()
        pe = struct.unpack_from('<I', binary, 60)[0]
        need(struct.unpack_from('<H', binary, pe + 4)[0] == 0x8664 and
             struct.unpack_from('<H', binary, pe + 24)[0] == 0x20b, name + ' AMD64/PE32+')
        execute([DUMP, '/imports', exe], run / (name + '-imports.txt'))
    unit = execute([test, session, run / 'fixtures'], run / 'extent-test.log')
    need('output sizing passed' in unit and 'preview_sizes=9 preview_guards=4' in unit,
         'extent test completion')
    fixture = run / 'fixtures/live-1280x720.dmx'
    clean = {k: v for k, v in os.environ.items() if not k.startswith('SFM64_')}
    captures = {}
    for name, executable, width, height, options in [
        ('preview', editor, 512, 288, {}),
        ('square-control', editor, 512, 512, {'SFM64_DIAGNOSTIC_SCENE_PREVIEW': '0'}),
        ('movie', editor, 1280, 720, {'SFM64_DIAGNOSTIC_SCENE_OUTPUT': 'movie'}),
        ('missing-settings', editor, 512, 512, {}),
        ('baseline-square', baseline, 512, 512, {})]:
        directory = run / name
        directory.mkdir()
        env = dict(clean, SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',
                   SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(directory), **options)
        input_session = run / 'fixtures/missing-movie-settings.dmx' if name == 'missing-settings' else fixture
        log = execute([executable, '--control-test', '--session', input_session,
                       '--content-directory', CONTENT, '--start-frame', '119'],
                      directory / 'run.log', env)
        need(f'scene_output_extent={width}x{height}' in log, name + ' real launch extent')
        need('model_gpu_presentation_passed=1 frames=5' in log and
             'gpu_readback_copies=5 capture_frames=1' in log, name + ' GPU capture path')
        frames = {}
        for frame in (0, 119, 120):
            path = directory / f'vulkan-retained-frame-{frame}.rgba'
            data = path.read_bytes()
            need(len(data) == width * height * 4, name + ' frame size')
            if frame:
                need(len(set(zip(data[0::4], data[1::4], data[2::4]))) > 16,
                     name + ' nonblank scene pixels')
            frames[str(frame)] = sha(path)
        need(frames['119'] != frames['120'], name + ' animation changed pixels')
        bitmap(directory / 'vulkan-retained-frame-120.rgba', width, height,
               directory / 'frame-120.bmp')
        captures[name] = dict(width=width, height=height, frame_sha256=frames)
    need(captures['square-control']['frame_sha256'] ==
         captures['baseline-square']['frame_sha256'], 'square renderer regression')
    need(captures['missing-settings']['frame_sha256'] ==
         captures['baseline-square']['frame_sha256'], 'missing-settings renderer regression')
    need(sha(baseline) == baseline_hash, 'last known editor preserved')
    need(all(sha(p) == h for p, h in original_hashes.items()), 'original inputs unchanged')
    need(all(sha(p) == h for p, h in sources.items()), 'sources changed during verification')
    warnings = re.findall(r'warning (C\d+): ([^\r\n]+)',
                         (run / 'build.log').read_text(errors='replace'))
    result = dict(status='VERIFIED',scope='session preview aspect, not full SFM parity',
                  executable=str(editor), executable_sha256=sha(editor),
                  architecture='AMD64 PE32+', compiler_warnings=warnings,
                  captures=captures, square_regression='three frames bitwise equal',
                  protected_inputs=original_hashes, source_sha256=sources,
                  baseline_executable_sha256=baseline_hash)
    receipt = run / 'results.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    (OUT / 'latest-verification.json').write_text(json.dumps(dict(
        receipt=str(receipt), receipt_sha256=sha(receipt)), indent=2) + '\n')
    print(json.dumps(dict(status=result['status'], receipt=str(receipt),
                         executable=str(editor), compiler_warning_count=len(warnings)), indent=2))


if __name__ == '__main__':
    main()
