"""Package only the owned editor and explicitly listed, owned shader artifacts."""
import hashlib
from pathlib import Path
import re
import struct
import zipfile


def shader_manifest(root):
    rows = []
    for line in (Path(root) / 'work/editor_shader_manifest.txt').read_text().splitlines():
        if not line.strip() or line.lstrip().startswith('#'):
            continue
        profile, stem = line.split()
        if profile not in {'vs_6_0', 'ps_6_0', 'cs_6_0'} or not re.fullmatch(r'sfm64-[a-z0-9-]+', stem):
            raise ValueError('Invalid owned shader manifest entry')
        if not (Path(root) / 'work' / (stem + '.hlsl')).is_file():
            raise ValueError('Owned shader source missing: ' + stem)
        rows.append((profile, stem))
    if not rows or len({stem for _, stem in rows}) != len(rows):
        raise ValueError('Empty or duplicate owned shader manifest')
    return rows


def owned_shader_payload(root, build):
    result = {}
    for _, stem in shader_manifest(root):
        data = (Path(build) / 'shaders' / (stem + '.spv')).read_bytes()
        if len(data) < 20 or len(data) % 4 or struct.unpack_from('<2I', data) != (0x07230203, 0x00010000):
            raise ValueError('Expected compiled SPIR-V 1.0: ' + stem)
        result['shaders/' + stem + '.spv'] = data
    return result


def write_editor_package(destination, executable, shaders, build_json, running, notices):
    # No directory globbing: only the validated shader map and these fixed files.
    if any(not re.fullmatch(r'shaders/sfm64-[a-z0-9-]+\.spv', name) for name in shaders):
        raise ValueError('Unexpected runtime shader path')
    payload = {'sfm64_session_ui.exe': executable, 'BUILD.json': build_json,
               'RUNNING.txt': running, 'THIRD_PARTY_NOTICES.md': notices, **shaders}
    payload['SHA256SUMS.txt'] = ''.join(hashlib.sha256(data).hexdigest() + '  ' + name + '\n'
                                      for name, data in sorted(payload.items())).encode()
    with zipfile.ZipFile(destination, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
        for name, data in sorted(payload.items()):
            entry = zipfile.ZipInfo(name, (2026, 10, 18, 0, 0, 0))
            entry.compress_type = zipfile.ZIP_DEFLATED
            archive.writestr(entry, data)
    with zipfile.ZipFile(destination) as archive:
        if archive.testzip() is not None or set(archive.namelist()) != set(payload):
            raise ValueError('Runtime ZIP integrity failure')
        for name, data in payload.items():
            if archive.read(name) != data:
                raise ValueError('Runtime ZIP content mismatch: ' + name)
    return {name: hashlib.sha256(data).hexdigest() for name, data in payload.items()}
