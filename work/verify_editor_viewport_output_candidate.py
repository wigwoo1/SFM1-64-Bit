"""Actual-editor HDR2 output pixels, copy lifetime and default regression.

Takes the directory containing fresh baseline-editor and candidate-editor builds.
Uses only generated sessions plus a read-only copy of the installed benchmark.
"""
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import math
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from verify_texture_sampling_cross_api import fixture as vtf_fixture

ROOT = Path(__file__).resolve().parent.parent


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def encoded(value):
    linear = min(1.0, max(0.0, value))
    result = 12.92 * linear if linear <= .0031308 else 1.055 * linear ** (1 / 2.4) - .055
    return math.floor(result * 255 + .5)


def main():
    need(len(sys.argv) == 2, 'Supply a directory containing both complete editor builds')
    parent = Path(sys.argv[1]).resolve()
    need(parent.is_relative_to(ROOT / 'outputs'), 'Use an isolated workspace output directory')
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=parent))
    before = json.loads((parent / 'before.json').read_text())
    baseline = parent / 'baseline-editor/sfm64_session_ui.exe'
    candidate = parent / 'candidate-editor/sfm64_session_ui.exe'
    clean = {key: value for key, value in os.environ.items() if not key.startswith('SFM64_')}
    result = dict(status='IN PROGRESS', time_utc=datetime.now(timezone.utc).isoformat(),
                  commands=[], captures={}, metrics={}, original_sha256={})

    def execute(arguments, label, environment=None, expected=0, cwd=ROOT):
        process = subprocess.run(list(map(str, arguments)), cwd=cwd, env=environment or clean,
                                 capture_output=True, text=True, timeout=240,
                                 creationflags=subprocess.CREATE_NO_WINDOW)
        text = process.stdout + process.stderr
        (run / (label + '.log')).write_text(text, encoding='utf-8')
        result['commands'].append(dict(arguments=list(map(str, arguments)), exit_code=process.returncode,
                                       expected_exit=expected))
        need(process.returncode == expected, label + ' failed ' + text[-2000:])
        return text

    def capture(binary, label, arguments, extra=None, hdr=False, pixels=True, expected=0):
        out = run / label
        out.mkdir()
        env = dict(clean, SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1' if pixels else '0',
                   SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(out))
        env.update(extra or {})
        text = execute([binary, *arguments], label, env, expected, foreign)
        files = {path.name: sha(path) for path in out.glob('*.rgba')}
        result['captures'][label] = files
        if expected:
            need(not files, 'Failed setup wrote a capture')
            return out, text, files
        need('model_gpu_presentation_passed=1 frames=5' in text, 'Five actual editor frames ' + label)
        need('gpu_readback_copies=' + ('5' if pixels else '0') in text, 'Readback policy ' + label)
        if hdr:
            need(text.count('hdr2_output_copy=1 ') == 1 and text.count('editor_hdr2_draw_fence_completed=') == 5,
                 'One retained output resource and five completed fences ' + label)
            need('hdr2_presentation_encoded_copy=1 capture=encoded_UNORM second_encoding=0' in text,
                 'UNORM presentation route ' + label)
            need('hdr2_presentation_srgb_blit=' not in text, 'Old second-encoding route ' + label)
        else:
            need('hdr2_output_copy=1 ' not in text, 'Default route unexpectedly changed')
        need(bool(files) == pixels, 'Actual capture availability ' + label)
        print('PASS actual editor ' + label, flush=True)
        return out, text, files

    try:
        foreign = run / 'empty-cwd'
        foreign.mkdir()
        for binary in [baseline, candidate]:
            data = binary.read_bytes()
            offset = struct.unpack_from('<I', data, 60)[0]
            need(data[offset:offset + 4] == b'PE\0\0' and
                 struct.unpack_from('<H', data, offset + 4)[0] == 0x8664 and
                 struct.unpack_from('<H', data, offset + 24)[0] == 0x20b, 'AMD64 PE32+')
        result['executables'] = {name: dict(path=str(path), sha256=sha(path))
                                 for name, path in [('baseline', baseline), ('candidate', candidate)]}
        warning_sets = {}
        for name, binary in [('baseline', baseline), ('candidate', candidate)]:
            log = (binary.parent / 'build.log').read_text(errors='replace')
            warnings = re.findall(r'([^\\\r\n]+\.h)\(\d+\): warning (C\d+): ([^\r\n]+)', log)
            warning_sets[name] = Counter(warnings)
            result.setdefault('compiler_warnings', {})[name] = warnings
            need(not re.search(r'error (C\d+|LNK\d+):', log), 'Build error')
            need(not (binary.parent / 'shader-build.log').read_text().strip(), 'Shader compiler diagnostic')
        need(warning_sets['candidate'] == warning_sets['baseline'], 'New compiler warnings')
        old_shaders = {p.name: sha(p) for p in (baseline.parent / 'shaders').glob('*.spv')}
        shaders = {p.name: sha(p) for p in (candidate.parent / 'shaders').glob('*.spv')}
        need(set(shaders) - set(old_shaders) == {'sfm64-viewport-copy-vertex.spv', 'sfm64-viewport-copy-fragment.spv'}, 'Only two new shaders')
        need(all(shaders[name] == digest for name, digest in old_shaders.items()), 'Unrelated shader changed')
        result['shader_sha256'] = shaders
        white = bytearray(vtf_fixture(0, 0)[:88])
        struct.pack_into('<HH', white, 16, 2, 2)
        struct.pack_into('<I', white, 20, 0x101)
        struct.pack_into('<H', white, 24, 1)
        white[56] = 1
        (run / 'white.vtf').write_bytes(white + bytes([255] * 16))
        execute(['cmd.exe', '/d', '/c', ROOT / 'work/build_editor_hdr2_profile_test.cmd', run / 'profile-build'], 'build-profile')
        profile_test = run / 'profile-build/test_editor_hdr2_profile_x64.exe'
        log = execute([profile_test, run / 'owned', run / 'white.vtf'], 'profile-fixtures')
        need('invalid_profiles=16 fixture_roundtrip=2' in log, 'Profile test coverage')
        owned = run / 'owned'
        for label, exposure in [('dark', .00390625), ('clipped', 8)]:
            text = (owned / '113-nearest-1.profile').read_text().replace('exposure 1\n', f'exposure {exposure}\n')
            (owned / (label + '.profile')).write_text(text)
        owned_before = {str(path.relative_to(owned)): sha(path) for path in owned.rglob('*') if path.is_file()}
        args = ['--control-test', '--session', owned / 'owned.dmx', '--content-directory', owned]
        default_base, _, hashes = capture(baseline, 'owned-default-baseline', args)
        _, _, candidate_hashes = capture(candidate, 'owned-default-candidate', args)
        need(hashes == candidate_hashes, 'Default owned pixels changed')
        # HDR2 admits a separate per-face subset, so its coverage must be
        # compared with the pre-change HDR2 route, not the default preview.
        mask_out, _, _ = capture(baseline, 'owned-hdr2-baseline', args,
            {'SFM64_EXPERIMENTAL_HDR2_PROFILE': str(owned / '113-nearest-1.profile')})
        masks = {}
        for path in mask_out.glob('*.rgba'):
            data = path.read_bytes()
            need(len(data) == 512 * 512 * 4, 'Capture extent')
            masks[path.name] = list(zip(*[iter(data)] * 4))
            for pixel in Counter(masks[path.name]):
                need(min(max(abs(a-b) for a,b in zip(pixel, color)) for color in
                     [(13,13,20,255), (64,0,0,255), (0,96,0,255)]) <= 1,
                     'Pre-change HDR2 literal fixture color ' + str(pixel))
        clears = tuple(encoded(value) for value in [.05, .05, .08]) + (255,)
        # Values come from the authored white texels, style .5, instance .5/.75
        # and literal red/green texlights, independently of renderer helpers.
        cases = [('half', '113-nearest-1.profile', 1, False),
                 ('exposure2', '113-nearest-2.profile', 2, False),
                 ('dark', 'dark.profile', .00390625, False),
                 ('clipped', 'clipped.profile', 8, False),
                 ('numeric', '36-linear-1.profile', 1, True)]
        repeated = None
        for label, profile, exposure, numeric in cases:
            out, _, files = capture(candidate, label, args,
                {'SFM64_EXPERIMENTAL_HDR2_PROFILE': str(owned / profile)}, True)
            if label == 'half': repeated = files
            for path in out.glob('*.rgba'):
                data = path.read_bytes()
                pixels = list(zip(*[iter(data)] * 4))
                mask = masks[path.name]
                need(len(pixels) == len(mask), 'Changed capture dimensions')
                colors = {'clear': clears, 'red': (encoded(.25 * exposure), 0, 0, 255),
                          'green': (0, encoded(.375 * exposure), 0, 255)}
                counts = Counter()
                maximum = 0
                for index, (pixel, previous) in enumerate(zip(pixels, mask)):
                    if previous == (13, 13, 20, 255): key = 'clear'
                    elif previous[0] and not previous[1] and not previous[2]: key = 'red'
                    elif previous[1] and not previous[0] and not previous[2]: key = 'green'
                    else: raise RuntimeError('Unexpected fixture baseline color ' + str(previous))
                    expected = (0, 0, 0, 255) if numeric and key != 'clear' else colors[key]
                    error = max(abs(a - b) for a, b in zip(pixel, expected))
                    need(error <= 1, f'{label} pixel {index} got {pixel}, expected {expected}')
                    maximum = max(maximum, error)
                    counts[key] += 1
                need(all(counts[key] > 0 for key in ['clear', 'red', 'green']), 'Nonempty geometry and clear coverage')
                result['metrics'][label + '/' + path.name] = dict(pixels=len(pixels), counts=dict(counts), max_error_byte=maximum)
        _, _, repeated_again = capture(candidate, 'half-repeat', args,
            {'SFM64_EXPERIMENTAL_HDR2_PROFILE': str(owned / '113-nearest-1.profile')}, True)
        need(repeated == repeated_again, 'Repeated encoded editor frames differ')
        capture(candidate, 'gpu-only', args,
                {'SFM64_EXPERIMENTAL_HDR2_PROFILE': str(owned / '113-nearest-1.profile')}, True, False)
        ticks = run / 'ticks.txt'
        ticks.write_text('0\n416\n10000\n')
        blank, text, _ = capture(candidate, 'blank-restore', ['--control-test'], dict(
            SFM64_EXPERIMENTAL_HDR2_PROFILE=str(owned / '113-nearest-1.profile'),
            SFM64_DIAGNOSTIC_FROZEN_SCENE_SESSION=str(owned / 'owned.dmx'),
            SFM64_DIAGNOSTIC_FROZEN_SCENE_TICK='0', SFM64_DIAGNOSTIC_SCENE_ROOT='1',
            SFM64_DIAGNOSTIC_CONTENT_DIRECTORY=str(owned), SFM64_DIAGNOSTIC_SCENE_TICKS=str(ticks),
            SFM64_DIAGNOSTIC_TIMELINE_BLANKS='1', SFM64_DIAGNOSTIC_SCENE_PREVIEW='1',
            SFM64_DIAGNOSTIC_BOUNDARY_CONTROL_TEST='1'), True)
        need('editor_hdr2_frame=2 faces=0 opaque_batches=0 translucent_batches=0' in text, 'Real blank frame visit')
        need((blank / 'vulkan-retained-frame-2.rgba').read_bytes() == bytes(clears) * (512 * 512), 'Stale output on blank frame')
        broken = run / 'missing-shader-build'
        (broken / 'shaders').mkdir(parents=True)
        shutil.copyfile(candidate, broken / candidate.name)
        for path in (candidate.parent / 'shaders').glob('*.spv'):
            if path.name != 'sfm64-viewport-copy-fragment.spv': shutil.copyfile(path, broken / 'shaders' / path.name)
        _, text, _ = capture(broken / candidate.name, 'missing-shader', args,
            {'SFM64_EXPERIMENTAL_HDR2_PROFILE': str(owned / '113-nearest-1.profile')}, True, True, 1)
        need('viewport output shader unavailable' in text, 'Precise setup failure')
        game = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
        source = game / 'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
        originals = [source, game / 'tf_movies/maps/mtt_soldier.bsp', game / 'usermod/gameinfo.txt']
        result['original_sha256'] = {str(path): sha(path) for path in originals}
        session = run / 'benchmark-read-copy.dmx'
        shutil.copyfile(source, session)
        benchmark_args = ['--control-test', '--session', session, '--content-directory', game / 'usermod', '--start-frame', '119']
        _, _, old_frames = capture(baseline, 'benchmark-default-baseline', benchmark_args)
        _, _, new_frames = capture(candidate, 'benchmark-default-candidate', benchmark_args)
        need(old_frames == new_frames and all(f'vulkan-retained-frame-{index}.rgba' in new_frames for index in [0, 119, 120]), 'Default benchmark regression')
        capture(candidate, 'benchmark-hdr2', benchmark_args,
                {'SFM64_EXPERIMENTAL_HDR2_PROFILE': str(owned / '113-nearest-1.profile')}, True)
        need(sha(session) == result['original_sha256'][str(source)], 'Disposable benchmark input changed')
        need(all(sha(path) == digest for path, digest in result['original_sha256'].items()), 'Original read-only evidence changed')
        need(all(sha(owned / path) == digest for path, digest in owned_before.items()), 'Owned input changed')
        for name, digest in before['source_sha256'].items():
            if name.replace('\\', '/') not in before['changed']: need(sha(ROOT / name) == digest, 'Unrelated source changed ' + name)
        result.update(status='VERIFIED actual editor experimental HDR2 encoded output; native visual parity incomplete',
                      baseline_source_sha256=before['source_sha256'],
                      changed_source_sha256={name: sha(ROOT / name) for name in before['changed']},
                      new_source_sha256={name: sha(ROOT / 'work' / name) for name in [
                          'vulkan_viewport_output_copy_candidate.h', 'sfm64-viewport-copy-vertex.hlsl',
                          'sfm64-viewport-copy-fragment.hlsl', Path(__file__).name]},
                      limits=['Explicit experimental profile only; default preview unchanged.',
                              'Controlled opaque scene RGB and clear output; native benchmark appearance, alpha and translucent scene parity remain incomplete.',
                              'Presented image routing is exercised; swapchain pixels are not read back by this test.',
                              'No Vulkan validation layer installed or enabled. Original SFM never executed or controlled.'])
    except Exception as error:
        result.update(status='PARTIALLY VERIFIED', failure=repr(error))
        raise
    finally:
        receipt = run / 'results.json'
        receipt.write_text(json.dumps(result, indent=2) + '\n')
        print('receipt=' + str(receipt), flush=True)
    print(json.dumps({'status': result['status'], 'editor': result['executables']['candidate'],
                      'literal_pixel_checks': sum(item['pixels'] for item in result['metrics'].values()),
                      'compiler_warning_count': len(result['compiler_warnings']['candidate'])}, indent=2))


if __name__ == '__main__':
    main()
