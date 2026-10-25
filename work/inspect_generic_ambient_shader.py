"""Offline extraction of one installed generic VS20 block; no GPU/SFM access."""
from pathlib import Path
import ctypes
from ctypes import wintypes
import hashlib
import json
import struct
from vcs6_shader_cache_candidate import Vcs6ShaderCache

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / 'outputs/generic-ambient-shader-20261024'
SOURCE = Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\platform\shaders\fxc\vertexlit_and_unlit_generic_vs20.vcs')


def disassemble(shader):
    compiler = ctypes.WinDLL('d3dcompiler_47.dll')
    function = compiler.D3DDisassemble
    function.argtypes = [ctypes.c_void_p, ctypes.c_size_t, wintypes.UINT, ctypes.c_char_p, ctypes.POINTER(ctypes.c_void_p)]
    function.restype = ctypes.c_long
    memory = ctypes.create_string_buffer(shader)
    blob = ctypes.c_void_p()
    result = function(memory, len(shader), 0, None, ctypes.byref(blob))
    if result < 0 or not blob.value:
        raise RuntimeError('D3DDisassemble failed: ' + str(result))
    table = ctypes.cast(blob, ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p))).contents
    get_buffer = ctypes.WINFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)(table[3])
    get_size = ctypes.WINFUNCTYPE(ctypes.c_size_t, ctypes.c_void_p)(table[4])
    release = ctypes.WINFUNCTYPE(wintypes.ULONG, ctypes.c_void_p)(table[2])
    try:
        return ctypes.string_at(get_buffer(blob), get_size(blob)).rstrip(b'\0').decode('utf-8')
    finally:
        release(blob)


def extract():
    OUT.mkdir(exist_ok=True)
    source = SOURCE.read_bytes()
    digest = hashlib.sha256(source).hexdigest()
    prior = json.loads((ROOT / 'outputs/sfm64-generic-lighting-draw-validation.json').read_text())
    if digest != prior['cache_sha256']:
        raise RuntimeError('Installed generic VS20 changed; reassess contract')
    count = struct.unpack_from('<I', source, 20)[0]
    entries = [struct.unpack_from('<II', source, 28 + 8 * i) for i in range(count)]
    i = next(i for i, (key, _) in enumerate(entries[:-1]) if key == 0)
    payload = source[entries[i][1]:entries[i+1][1]]
    # Preserve the native compression and dynamic records in one-primary cache.
    fixture = source[:20] + struct.pack('<II', 2, struct.unpack_from('<I', source, 24)[0])
    fixture += struct.pack('<II', 0, 48) + struct.pack('<II', 0xffffffff, 48 + len(payload))
    cache = Vcs6ShaderCache(fixture + struct.pack('<I', 0) + payload)
    programs = []
    for dynamic in [2, 6, 10, 18]:
        shader = cache.shader(0, dynamic)
        stem = 'generic-vs20-s0-d' + str(dynamic)
        (OUT / (stem + '.bin')).write_bytes(shader)
        (OUT / (stem + '.asm')).write_text(disassemble(shader), encoding='utf-8')
        programs.append(dict(dynamic=dynamic, bytecode_sha256=hashlib.sha256(shader).hexdigest(),
                             bytecode=stem + '.bin', disassembly=stem + '.asm'))
    if SOURCE.read_bytes() != source:
        raise RuntimeError('Installed cache changed during read')
    receipt = dict(source=str(SOURCE), source_sha256=digest, programs=programs,
                   scope='Explicit static0/dynamic2,6,10,18; no claim these are benchmark runtime selections',
                   original_unchanged=True, sfm_process_used=False, gpu_used=False)
    (OUT / 'extraction.json').write_text(json.dumps(receipt, indent=2) + '\n')
    return receipt


if __name__ == '__main__':
    print(json.dumps(extract()))
