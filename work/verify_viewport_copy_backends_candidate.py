"""Fresh x64 builds and controlled D3D9/Vulkan viewport-copy pixel comparison."""
from datetime import datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
DUMP = Path(r'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\dumpbin.exe')
SOURCES = [
    'work/viewport_copy_fixture_candidate.h',
    'work/probe_viewport_copy_srgb_x64.cpp',
    'work/build_viewport_copy_srgb_x64.cmd',
    'work/probe_viewport_copy_vulkan_x64.cpp',
    'work/viewport_copy_vulkan_candidate.hlsl',
    'work/build_viewport_copy_vulkan_x64.cmd',
    'work/verify_viewport_copy_backends_candidate.py',
]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def editor_dependencies():
    pending = [ROOT / 'work/probe_vulkan_win32_surface_x64.cpp']
    found = set()
    while pending:
        path = pending.pop().resolve()
        if path in found:
            continue
        found.add(path)
        for name in re.findall(r'^\s*#\s*include\s*"([^"]+)"', path.read_text(errors='replace'), re.M):
            for candidate in [path.parent / name, ROOT / 'work' / name]:
                if candidate.is_file():
                    pending.append(candidate)
                    break
    return found


def expected_pixels():
    f32 = lambda value: struct.unpack('<f', struct.pack('<f', value))[0]
    values = [f32(index / 255.0) for index in range(256)]
    values += [f32(value) for value in [-0.5, -0.0, 0.0001, 0.003, 0.0031308, 0.0032, 0.18, 0.5, 1.5, 4.0]]

    def encode(value):
        linear = min(1.0, max(0.0, value))
        encoded = 12.92 * linear if linear <= 0.0031308 else 1.055 * linear**(1.0 / 2.4) - 0.055
        return math.floor(encoded * 255.0 + 0.5)

    return [tuple(encode(values[(index + offset) % len(values)]) for offset in [0, 47, 137])
            for index in range(len(values))]


def read_capture(text, backend, formats):
    rows = {}
    for line in text.splitlines():
        if not line.startswith('pixel '):
            continue
        columns = line.split()
        require(len(columns) == 10 and columns[1] == backend, 'Unexpected pixel row')
        key = tuple(map(int, columns[2:4]))
        require(key not in rows, 'Duplicate captured pixel')
        values = tuple(map(int, columns[4:]))
        require(all(0 <= value <= 255 for value in values), 'Captured byte outside range')
        require(values[:3] == values[3:], 'Second copy changed stored RGB values')
        rows[key] = values
    expected = expected_pixels()
    require(set(rows) == {(fmt, index) for fmt in formats for index in range(len(expected))},
            'Missing, extra or misplaced captured pixel')
    errors = [abs(rows[fmt, index][channel] - pixel[channel])
              for fmt in formats for index, pixel in enumerate(expected) for channel in range(3)]
    require(max(errors) <= 1, 'Reference encoding error exceeds predeclared one-byte tolerance')
    require('PASS native_x64=1' in text and 'encoding_checks=1596 copy_checks=1596' in text,
            'GPU fixture did not complete its checks')
    require(text.count('wrong_read_changed_pixels=266 wrong_write_changed_pixels=266') == 2,
            'Misplaced conversion controls failed')
    return rows, max(errors)


def main():
    before = {name: sha(ROOT / name) for name in SOURCES}
    dependencies = editor_dependencies()
    require(not dependencies.intersection((ROOT / name).resolve() for name in SOURCES),
            'Fixture unexpectedly entered the editor include graph')
    editor_before = {str(path.relative_to(ROOT)): sha(path) for path in dependencies}
    run = Path(tempfile.mkdtemp(prefix='viewport-copy-backends-', dir=ROOT / 'outputs'))
    result = dict(status='IN PROGRESS', checked_at_utc=datetime.now(timezone.utc).isoformat(),
                  preceding_goal_turn='PROGRESS: output-copy producer/consumer and native D3D9 GPU sequence verified',
                  source_sha256=before, editor_include_sha256=editor_before, commands=[], executables=[])

    def execute(arguments, label, timeout=90):
        arguments = list(map(str, arguments))
        process = subprocess.run(arguments, cwd=ROOT, capture_output=True, text=True,
                                 timeout=timeout, creationflags=subprocess.CREATE_NO_WINDOW)
        (run / (label + '.log')).write_text(process.stdout + process.stderr, encoding='utf-8')
        result['commands'].append(dict(arguments=arguments, exit_code=process.returncode))
        require(process.returncode == 0, label + ' failed ' + (process.stdout + process.stderr)[-1600:])
        return process.stdout + process.stderr

    try:
        captures = {}
        errors = {}
        for backend, script, name, formats in [
            ('d3d9', 'build_viewport_copy_srgb_x64.cmd', 'viewport_copy_srgb_x64.exe', [21, 22]),
            ('vulkan', 'build_viewport_copy_vulkan_x64.cmd', 'viewport_copy_vulkan_x64.exe', [37, 44]),
        ]:
            output = run / backend
            execute(['cmd.exe', '/d', '/c', ROOT / 'work' / script, output], backend + '-build')
            build_log = (output / 'build.log').read_text(errors='replace')
            require(not re.search(r'\b(?:warning|error) [A-Z]+\d+:', build_log), 'Compiler diagnostic')
            if backend == 'vulkan':
                shader_log = (output / 'shader-build.log').read_text(errors='replace')
                require('warning:' not in shader_log.lower(), 'Shader compiler warning')
                for shader in ['copy-vertex.spv', 'copy-fragment.spv']:
                    raw = (output / shader).read_bytes()
                    require(len(raw) >= 20 and len(raw) % 4 == 0 and raw[:4] == b'\x03\x02\x23\x07', 'SPIR-V framing')
            executable = output / name
            raw = executable.read_bytes()
            offset = struct.unpack_from('<I', raw, 60)[0]
            require(raw[offset:offset + 4] == b'PE\0\0'
                    and struct.unpack_from('<H', raw, offset + 4)[0] == 0x8664
                    and struct.unpack_from('<H', raw, offset + 24)[0] == 0x20B
                    and b'asInvoker' in raw, 'Native x64 executable/manifest')
            imports_text = execute([DUMP, '/imports', executable], backend + '-imports')
            imports = re.findall(r'^\s+([A-Za-z0-9_.-]+\.dll)\s*$', imports_text, re.M)
            allowed = {'kernel32.dll', 'user32.dll', 'd3d9.dll'} if backend == 'd3d9' else {'kernel32.dll'}
            require(set(map(str.lower, imports)) <= allowed, 'Unexpected direct runtime dependency')
            result['executables'].append(dict(backend=backend, path=str(executable), sha256=sha(executable),
                                              machine='AMD64 PE32+', direct_imports=imports))
            arguments = [executable]
            if backend == 'vulkan':
                arguments.append(output)
            text = execute(arguments, backend + '-gpu', timeout=45)
            captures[backend], errors[backend] = read_capture(text, backend, formats)
            if backend == 'vulkan':
                require('validation_errors=0' in text and 'validation ' not in text, 'Vulkan validation message')
                result['vulkan_validation_layer_available'] = 'validation_layer=1' in text
        comparisons = []
        for vulkan_format in [37, 44]:
            for d3d_format in [21, 22]:
                differences = [abs(a - b) for index in range(266)
                               for a, b in zip(captures['vulkan'][vulkan_format, index],
                                               captures['d3d9'][d3d_format, index])]
                require(max(differences) <= 1, 'API comparison exceeds predeclared one-byte tolerance')
                comparisons.append(dict(vulkan_format=vulkan_format, d3d9_format=d3d_format,
                                        channel_comparisons=len(differences),
                                        different_channels=sum(value != 0 for value in differences),
                                        maximum_byte_difference=max(differences)))
        require(all(sha(ROOT / path) == digest for path, digest in before.items()), 'Fixture source changed during test')
        require(all(sha(ROOT / path) == digest for path, digest in editor_before.items()), 'Editor dependency changed during test')
        result.update(status='PASS controlled native x64 D3D9/Vulkan copy parity; full application PARTIALLY VERIFIED',
                      per_backend_encoding_checks=1596, per_backend_exact_copy_checks=1596,
                      reference_encoding_tolerance_byte=1, maximum_reference_error=errors,
                      exact_copy_tolerance_byte=0, api_tolerance_byte=1, api_comparisons=comparisons,
                      compiler_warnings=[], editor_build='NOT RUN; fixture sources outside editor include graph',
                      shader_sha256={name: sha(run / 'vulkan' / name) for name in ['copy-vertex.spv', 'copy-fragment.spv']},
                      specification_references=['https://docs.vulkan.org/spec/latest/chapters/resources.html',
                                                'https://docs.vulkan.org/spec/latest/chapters/images.html'],
                      limits=['Explicit controlled API states and synthetic float32 RGB input. Original SFM is unused and F: is unnecessary.',
                              'Native live branch selection, final benchmark image, alpha, filtering, progressive accumulation, bloom and AO arithmetic are outside this fixture.',
                              'Default Khronos validation is used only when locally available. No validation layer is installed by this test.',
                              'RGB parity does not prove integration into the editor or compatibility across untested devices.'])
    except Exception as error:
        result.update(status='FAIL', failure=repr(error))
        raise
    finally:
        receipt = run / 'verification.json'
        receipt.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print('receipt=' + str(receipt), flush=True)
    print(json.dumps({key: result[key] for key in ['status', 'api_comparisons', 'vulkan_validation_layer_available']}, indent=2))


if __name__ == '__main__':
    main()
