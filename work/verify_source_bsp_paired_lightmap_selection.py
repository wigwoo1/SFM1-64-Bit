"""Build isolated x64 regressions for explicitly paired BSP lightmap datasets."""
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/paired-bsp-dataset-20261104'
VS = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat')
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
MAP = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\maps\mtt_soldier.bsp')
ALLOWED = {'work/source_bsp_lightmap_data_candidate.h',
           'work/source_bsp_inline_brush_candidate.h',
           'work/source_bsp_lightmap_binding_candidate.h'}

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def need(value, message):
    if not value:
        raise RuntimeError(message)

def pe(path):
    data = Path(path).read_bytes()
    at = struct.unpack_from('<I', data, 60)[0]
    return struct.unpack_from('<H', data, at + 4)[0], struct.unpack_from('<H', data, at + 24)[0]

def run(args, cwd, label, log):
    p = subprocess.run(list(map(str, args)), cwd=cwd, capture_output=True, text=True,
                       timeout=900, creationflags=subprocess.CREATE_NO_WINDOW)
    Path(log).write_text(p.stdout + p.stderr, encoding='utf-8')
    need(p.returncode == 0, f'{label} failed ({p.returncode}): {(p.stdout+p.stderr)[-2500:]}')
    return {'exit_code': p.returncode, 'output': p.stdout + p.stderr}

def main():
    before = json.loads((OUT / 'before.json').read_text())
    run_dir = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    result = {'status': 'IN PROGRESS', 'time_utc': datetime.now(timezone.utc).isoformat(),
              'run_directory': str(run_dir), 'baseline': before, 'tests': {}, 'transitions': {}}
    receipt = run_dir / 'results.json'
    try:
        baseline = before['baseline_sha256']
        editor_baseline = baseline['outputs/sfm64-editor-x64/sfm64_session_ui.exe']
        for path, expected in baseline.items():
            if path in ALLOWED:
                backup = OUT/(Path(path).name+'.before')
                need(sha(backup) == expected, 'saved before-source identity mismatch: '+path)
            else:
                need(sha(path) == expected, 'baseline identity changed before test: ' + path)
        need(MAP.is_file(), 'read-only mtt_soldier BSP missing')
        map_sha = sha(MAP)
        for path, expected in before['editor_dependency_sha256'].items():
            if path not in ALLOWED:
                need(sha(path) == expected, 'editor dependency changed before test: ' + path)

        paired_build = run_dir/'paired-selection-build'
        paired_build.mkdir()
        run(['cmd.exe','/d','/c',ROOT/'work/build_source_bsp_paired_lightmap_selection_test.cmd',paired_build],
            ROOT,'build paired selection test',run_dir/'paired-selection-build-driver.log')
        executables = {name: run_dir / (name + '.exe') for name in (
            'test_source_bsp_lightmap_data', 'test_source_bsp_lightmap_binding',
            'test_source_bsp_base_lightmap')}
        paired_exe = paired_build/'test_source_bsp_paired_lightmap_selection_x64.exe'
        lines = ['@echo off', f'cd /d "{ROOT}"', f'call "{VS}" >nul', 'if errorlevel 1 exit /b 1']
        sources = {
            'test_source_bsp_lightmap_data': 'test_source_bsp_lightmap_data.cpp',
            'test_source_bsp_lightmap_binding': 'test_source_bsp_lightmap_binding.cpp',
            'test_source_bsp_base_lightmap': 'test_source_bsp_base_lightmap.cpp'}
        for name, source in sources.items():
            lines += [f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\{source} /Fo:"{run_dir/(name+".obj")}" /Fe:"{executables[name]}" >"{run_dir/(name+"-build.log")}" 2>&1',
                      'if errorlevel 1 exit /b 1']
        compile_cmd = run_dir / 'compile-cpu-tests.cmd'
        compile_cmd.write_text('\n'.join(lines + ['exit /b 0', '']), encoding='ascii')
        run(['cmd.exe', '/d', '/c', compile_cmd], ROOT, 'compile CPU tests', run_dir/'compile-driver.log')
        result['tests']['compile'] = {}
        paired_log = (paired_build/'build.log').read_text(errors='replace')
        need(pe(paired_exe) == (0x8664, 0x20b), 'paired fixture is not AMD64 PE32+')
        result['tests']['compile']['test_source_bsp_paired_lightmap_selection'] = {
            'exe': str(paired_exe), 'sha256': sha(paired_exe), 'architecture': 'AMD64 PE32+',
            'warnings': re.findall(r'warning (C\d+): ([^\r\n]+)', paired_log)}
        for name, exe in executables.items():
            need(pe(exe) == (0x8664, 0x20b), name + ' is not AMD64 PE32+')
            build_log = (run_dir / (name + '-build.log')).read_text(errors='replace')
            result['tests']['compile'][name] = {'exe': str(exe), 'sha256': sha(exe),
                'architecture': 'AMD64 PE32+', 'warnings': re.findall(r'warning (C\d+): ([^\r\n]+)', build_log)}

        new_exe = paired_exe
        repeats = []
        for n in range(2):
            r = run([new_exe], ROOT, f'paired fixture repeat {n+1}', run_dir/f'paired-{n+1}.log')
            need('paired_bsp_dataset=PASS' in r['output'], 'paired fixture pass marker missing')
            repeats.append(r['output'])
        need(repeats[0] == repeats[1], 'paired fixture protocol output differs between runs')
        result['tests']['paired_fixture'] = {'repetitions': 2, 'identical_output': True,
            'output': repeats[0].strip(), 'exit_codes': [0, 0]}

        for name in ('test_source_bsp_lightmap_data', 'test_source_bsp_lightmap_binding'):
            fresh = run_dir / (name + '-fresh-output')
            need(not fresh.exists(), 'test output path must be fresh')
            r = run([executables[name], fresh, MAP], ROOT, name, run_dir/(name+'.log'))
            result['tests'][name] = {'exit_code': 0, 'output': r['output'].strip(),
                                     'fresh_disposable_output': str(fresh), 'map_sha256_read_only': map_sha}
        r = run([executables['test_source_bsp_base_lightmap']], ROOT, 'test_source_bsp_base_lightmap', run_dir/'test_source_bsp_base_lightmap.log')
        result['tests']['test_source_bsp_base_lightmap'] = {'exit_code': 0, 'output': r['output'].strip()}

        # Verify precisely the intended source transitions and preserve every
        # other original editor dependency and all pinned read-only references.
        for path, expected in baseline.items():
            if path in ALLOWED:
                result['transitions'][path] = {'before_sha256': expected, 'after_sha256': sha(path), 'changed': sha(path) != expected}
            else:
                need(sha(path) == expected, 'protected baseline changed: ' + path)
        new_enum = ROOT/'work/source_bsp_lightmap_selection_candidate.h'
        result['transitions'][str(new_enum)] = {'before': 'absent', 'after_sha256': sha(new_enum)}
        result['transitions'][str(ROOT/'work/test_source_bsp_paired_lightmap_selection.cpp')] = {'before': 'absent', 'after_sha256': sha(ROOT/'work/test_source_bsp_paired_lightmap_selection.cpp')}
        result['transitions'][str(ROOT/'work/build_source_bsp_paired_lightmap_selection_test.cmd')] = {'before': 'absent', 'after_sha256': sha(ROOT/'work/build_source_bsp_paired_lightmap_selection_test.cmd')}
        result['transitions'][str(ROOT/'work/verify_source_bsp_paired_lightmap_selection.py')] = {'before': 'absent', 'after_sha256': sha(__file__)}
        need(sha(MAP) == map_sha, 'read-only benchmark BSP changed')

        # A full editor rebuild is mandatory: the changed reader headers are in
        # its dependency closure. Its old canonical executable is never replaced.
        editor_out = run_dir / 'complete-editor-x64'
        editor_out.mkdir()
        r = run(['cmd.exe', '/d', '/c', ROOT/'work/build_sfm64_editor_x64.cmd', editor_out], ROOT,
                'complete editor build', run_dir/'complete-editor-driver.log')
        editor = editor_out/'sfm64_session_ui.exe'
        need(editor.is_file() and pe(editor) == (0x8664, 0x20b), 'complete editor AMD64 PE32+ check')
        editor_log = (editor_out/'build.log').read_text(errors='replace')
        shader_log = (editor_out/'shader-build.log').read_text(errors='replace')
        result['complete_editor_build'] = {'command': f'work\\build_sfm64_editor_x64.cmd {editor_out}',
            'exe': str(editor), 'sha256': sha(editor), 'architecture': 'AMD64 PE32+',
            'warnings': re.findall(r'warning (C\d+): ([^\r\n]+)', editor_log),
            'shader_build_exit_code': 0, 'shader_build_log': shader_log.strip(), 'exit_code': 0}
        result['canonical_editor_preserved_sha256'] = editor_baseline
        need(sha('outputs/sfm64-editor-x64/sfm64_session_ui.exe') == editor_baseline,
             'canonical editor executable changed')

        # Confirm old dependency closure changed only at the three intended
        # reader headers; the new shared enum becomes one additional include.
        status = json.loads((ROOT/'publication/stage/docs/build-status.json').read_text())
        changed_deps = [p for p,h in status['editor_source_dependencies'].items()
                        if p in before['editor_dependency_sha256'] and sha(p) != h]
        need(set(changed_deps) == ALLOWED, 'unexpected editor source dependency transitions: '+repr(changed_deps))
        result['complete_editor_source_dependency_changes'] = sorted(changed_deps)
        result['benchmark_read_only'] = {'path': str(MAP), 'sha256_before_after': map_sha}
        result['limits'] = ['Synthetic pair-selection behavior and isolated CPU regressions only; no editor runtime or visual lighting parity was tested.',
                            'The editor was rebuilt because its BSP reader headers changed; this does not integrate the HDR2 Vulkan draw path.',
                            'Original SFM installation and sessions were read-only.']
        result['status'] = 'VERIFIED paired explicit BSP reader selection; HDR2 renderer integration remains incomplete'
    except Exception as exc:
        result['status'] = 'PARTIALLY VERIFIED' if result['tests'] else 'BLOCKED'
        result['failure'] = repr(exc)
        raise
    finally:
        result['finished_utc'] = datetime.now(timezone.utc).isoformat()
        receipt.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        (OUT/'latest-verification.json').write_text(json.dumps({'receipt': str(receipt),
            'receipt_sha256': sha(receipt), 'status': result['status']}, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'status': result['status'], 'receipt': str(receipt),
                      'editor': result['complete_editor_build'],
                      'tests': result['tests']}, indent=2))

if __name__ == '__main__':
    main()
