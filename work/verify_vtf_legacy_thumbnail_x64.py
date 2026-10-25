"""Legacy VTF thumbnail layout regression; all fixture/save writes are disposable."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import os
import shutil
import struct
import subprocess
import tempfile
import zlib

root = Path(__file__).resolve().parent.parent
out = root / 'outputs/vtf-legacy-thumbnail-20261024'
run_dir = Path(tempfile.mkdtemp(prefix='verification-', dir=out))
exe = out / 'sfm64_session_ui_vtf_legacy.exe'
game = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
original = game / 'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
map_file = game / 'tf_movies/maps/mtt_soldier.bsp'
content = game / 'usermod'
tf = game / 'tf'
session = run_dir / 'input.dmx'
saved = run_dir / 'edited-copy.dmx'
inputs = [original, map_file, tf / 'materials/models/effects/pyro/pilotlight.vmt',
          tf / 'materials/models/effects/pyro/pilotlight.vtf',
          tf / 'materials/models/effects/pyro/pilotlight_motion.vtf']
texture_directory = content / 'materials/models/anno/mamagen'
inputs += [texture_directory / (name + '.vtf') for name in
           ('armor', 'head', 'visor', 'eyes', 'transparenteyes', 'fluff', 'bodysfw', 'visorglow')]
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
before = {}
records = []
status = 'RUNNING'
env = {k: v for k, v in os.environ.items()
       if not k.startswith('SFM64_DIAGNOSTIC_') and k != 'SFM64_SHOW_CAMERA_FRUSTUM'}
sources = ['vtf_texture_layout.h', 'vtf_block_decode.h', 'test_vtf_legacy_no_thumbnail.cpp',
           'test_vtf_texture_layout_x64.cpp', 'test_vtf_block_decode_x64.cpp',
           'test_vtf_animation.cpp', 'test_vtf73_texture.cpp',
           'model_render_inputs_candidate.h', 'normal_two_texture_material_candidate.h',
           'build_vtf_legacy_thumbnail_x64.cmd', 'verify_vtf_legacy_thumbnail_x64.py']
shaders = ['sfm64-diagnostic-model.spv', 'sfm64-material-alpha-candidate.spv',
           'sfm64-unlit-two-texture-candidate.spv']


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def receipt():
    report = dict(status=status, checked_at=datetime.now(timezone.utc).isoformat(),
        executable=str(exe), executable_sha256=sha(exe), records=records,
        inputs_before=before, inputs_after={str(p): sha(p) for p in inputs if p.is_file()},
        sources_sha256={name: sha(root / 'work' / name) for name in sources},
        shader_sha256={name: sha(root / 'build/session-core' / name) for name in shaders},
        limits=['Default/diagnostic regression only; no new Source visual parity.',
                'This corrects old thumbnail layout only; compressed decoding and material admission are unchanged.',
                'Original SFM and Blender are not launched.'])
    (run_dir / 'results.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


def process(name, command, settings=None):
    print('Starting ' + name, flush=True)
    log_path = run_dir / (name + '.log')
    with log_path.open('x') as log:
        child = subprocess.Popen([str(p) for p in command], cwd=root,
            env=dict(env, **(settings or {})), stdout=log, stderr=subprocess.STDOUT,
            creationflags=subprocess.CREATE_NO_WINDOW)
        try:
            code = child.wait(timeout=600)
        except subprocess.TimeoutExpired:
            child.kill()
            child.wait()
            raise
    records.append(dict(name=name, command=[str(p) for p in command], environment=settings,
        pid=child.pid, exit_code=code, executable_sha256=sha(command[0]), log=str(log_path)))
    receipt()
    text = log_path.read_text(errors='replace')
    require(code == 0, name + ' failed: ' + text[-2000:])
    print(name + ': passed', flush=True)
    return text


def png(path, rgba):
    width, height = 1280, 720
    require(len(rgba) == width * height * 4, 'GPU capture dimensions')
    def chunk(kind, data):
        body = kind + data
        return struct.pack('>I', len(data)) + body + struct.pack('>I', zlib.crc32(body) & 0xffffffff)
    rows = b''.join(b'\0' + rgba[y*width*4:(y+1)*width*4] for y in range(height))
    data = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0))
    data += chunk(b'IDAT', zlib.compress(rows, 6)) + chunk(b'IEND', b'')
    with path.open('xb') as stream:
        stream.write(data)


print('Results: ' + str(run_dir), flush=True)
try:
    raw = exe.read_bytes()
    pe = struct.unpack_from('<I', raw, 60)[0]
    require(raw[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H', raw, pe+4)[0] == 0x8664
            and struct.unpack_from('<H', raw, pe+24)[0] == 0x20b, 'AMD64 PE32+ required')
    baseline = json.loads((out / 'baseline.json').read_text())
    require(baseline['status'] == 'PASS_BASELINE_REPRODUCTION', 'Missing pre-fix evidence')
    for path, digest in baseline['sha256'].items():
        require(sha(root / path) == digest, 'Pre-fix evidence changed: ' + path)
    records.append(dict(name='baseline-evidence', receipt=str(out / 'baseline.json'), sha256=sha(out / 'baseline.json')))
    code_before = {name: sha(root / 'work' / name) for name in sources}
    shaders_before = {name: sha(root / 'build/session-core' / name) for name in shaders}
    before = {str(p): sha(p) for p in inputs}
    shutil.copyfile(original, session)
    inputs.append(session)
    before[str(session)] = sha(session)
    receipt()
    process('legacy-fixtures', [out / 'test_vtf_legacy_no_thumbnail.exe', run_dir / 'legacy-fixtures'])
    process('existing-layout', [out / 'test_vtf_texture_layout_x64.exe', texture_directory])
    process('existing-decode', [out / 'test_vtf_block_decode_x64.exe', tf / 'materials/models/effects/pyro/pilotlight.vtf', run_dir / 'pilot-mip2.rgba'])
    process('existing-animation', [out / 'test_vtf_animation.exe'])
    process('existing-resources', [out / 'test_vtf73_texture.exe', run_dir / 'legacy-fixtures/v7-3-header104-thumb.vtf'])
    records.append(dict(name='owned-fixture-hashes', sha256={str(p): sha(p) for p in (run_dir / 'legacy-fixtures').glob('*.vtf')}))
    for name in ('test_two_texture_bridge_resources', 'test_two_texture_draw_transport',
                 'test_vulkan_scene_commands', 'test_material_uv_transform'):
        process(name, [out / (name + '.exe')])
    process('two-texture-inputs', [out / 'test_unlit_two_texture_inputs.exe', run_dir / 'fixtures', tf])
    process('world-materials', [out / 'test_source_bsp_world_materials.exe', map_file, content / 'gameinfo.txt'])
    process('atomic-save', [out / 'test_atomic_session_save.exe', run_dir / 'atomic-save'])
    process('document-save', [out / 'test_session_document_save_copy.exe', run_dir / 'document-save'])
    process('benchmark-preparation', [out / 'test_mtt_soldier_scene_render_preparation.exe', session, content / 'gameinfo.txt'])

    comparisons = [
        ('default', {}, root / 'outputs/current-benchmark-workflow-20261024/verification-lf1x96g9/capture',
         root / 'outputs/current-benchmark-workflow-20261024/verification-lf1x96g9/results.json'),
        ('active', dict(SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST='1'),
         root / 'outputs/two-texture-editor-diagnostic-20261015/verification-0jd4yr8u/two-texture-active',
         root / 'outputs/two-texture-editor-diagnostic-20261015/verification-0jd4yr8u/results.json')]
    for name, settings, reference, prior in comparisons:
        frames = run_dir / name
        frames.mkdir()
        text = process(name, [exe, '--control-test', '--session', session, '--content-directory', content,
            '--start-frame', '120'], dict(settings, SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',
                SFM64_DIAGNOSTIC_SCENE_OUTPUT='movie', SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(frames)))
        require('retained Vulkan sequence passed frames=5' in text, 'Incomplete GPU sequence: ' + name)
        if name == 'active':
            require('two_texture_editor_materials=1' in text, 'Pilot material not admitted')
        for frame in (120, 121):
            file = 'vulkan-retained-frame-%d.rgba' % frame
            require((frames / file).read_bytes() == (reference / file).read_bytes(), 'Pixels changed: ' + name + file)
            records.append(dict(name=name + '-pixel-regression', frame=frame, unchanged=True,
                sha256=sha(frames / file), reference=str(reference / file),
                reference_receipt=str(prior), reference_receipt_sha256=sha(prior)))
        png(frames / 'vulkan-retained-frame-120.png', (frames / 'vulkan-retained-frame-120.rgba').read_bytes())
    edit = run_dir / 'edit-output'
    edit.mkdir()
    text = process('edit-save', [exe, '--control-test', '--session', session, '--content-directory', content,
        '--start-frame', '118'], dict(SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST='1',
            SFM64_DIAGNOSTIC_CAPTURE_FRAMES='0', SFM64_DIAGNOSTIC_EDITOR_CONTROL_TEST='1',
            SFM64_DIAGNOSTIC_EDITOR_SAVE_PATH=str(saved), SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(edit)))
    require('editor_session_saved=' in text and saved.is_file(), 'Saved copy missing')
    process('roundtrip-compare', [out / 'test_mtt_soldier_editor_roundtrip.exe', session, saved, content / 'gameinfo.txt'])
    reopen = run_dir / 'reopen-output'
    reopen.mkdir()
    text = process('reopen', [exe, '--control-test', '--session', saved, '--content-directory', content,
        '--start-frame', '119'], dict(SFM64_DIAGNOSTIC_TWO_TEXTURE_TEST='1', SFM64_DIAGNOSTIC_CAPTURE_FRAMES='0',
            SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(reopen)))
    require('retained Vulkan sequence passed frames=5' in text, 'Incomplete reopen')
    require(before == {str(p): sha(p) for p in inputs}, 'Original/input changed')
    require(code_before == {name: sha(root / 'work' / name) for name in sources}, 'Code changed during verification')
    require(shaders_before == {name: sha(root / 'build/session-core' / name) for name in shaders}, 'Shader changed during verification')
    records.append(dict(name='final', originals_unchanged=True, saved_copy=str(saved),
        saved_sha256=sha(saved), all_checks_passed=True))
    status = 'PASS'
    receipt()
    print('PASS: legacy VTF layouts/decodes, existing regressions, unchanged GPU frames, edit/save-copy/reopen', flush=True)
except BaseException as error:
    status = 'FAILED'
    records.append(dict(name='failure', error=str(error)))
    receipt()
    raise
