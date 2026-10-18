"""Run and independently inspect the isolated installed-shader x64 GPU oracle."""
from pathlib import Path
import csv
import hashlib
import json
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = ROOT/'outputs/unlit-two-texture-gpu-20261014'
EXE = OUTPUT/'probe_unlit_two_texture_d3d9.exe'
SHADERS = ROOT/'outputs/unlit-two-texture-contract-20261014/native-si802g4p'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def srgb_decode(value):
    value /= 255
    return value/12.92 if value <= .04045 else ((value+.055)/1.055)**2.4


def srgb_encode(value):
    return 255*(12.92*value if value <= .0031308 else 1.055*value**(1/2.4)-.055)


image = EXE.read_bytes()
pe = struct.unpack_from('<I', image, 60)[0]
require(image[pe:pe+4] == b'PE\0\0', 'Executable missing PE header')
require(struct.unpack_from('<H', image, pe+4)[0] == 0x8664, 'Executable is not AMD64')
require(struct.unpack_from('<H', image, pe+24)[0] == 0x20b, 'Executable is not PE32+')
source_names = ['unlittwotexture_vs20-s0-d2.bin', 'unlittwotexture_ps20-s0-d0.bin']
programs = {name: digest(SHADERS/name) for name in source_names}
extracted = json.loads((SHADERS/'evidence.json').read_text())
# Bytecode files must be exactly those recorded by the installed-cache extraction.
recorded_programs = {}
for cache in extracted['caches']:
    require(digest(Path(cache['path'])) == cache['sha256'], 'Installed shader cache changed')
    for entry in cache['programs']:
        recorded_programs[Path(entry['disassembly']).with_suffix('.bin').name] = entry['bytecode_sha256']
for name,value in programs.items():
    require(recorded_programs.get(name) == value, 'Shader file does not match its extraction record')
directory = Path(tempfile.mkdtemp(prefix='verified-', dir=OUTPUT))
run = subprocess.run([str(EXE), str(SHADERS), str(directory/'captures')], cwd=ROOT,
                     capture_output=True, text=True, timeout=45)
(directory/'runtime.log').write_text(run.stdout+run.stderr)
require(run.returncode == 0, 'GPU probe failed: '+run.stdout+run.stderr)
expected = {
    'linear-product': [64,32,16,255], 'second-uv-only': [32,64,32,255],
    'first-uv-only': [32,32,32,255], 'additive': [80,64,64,255],
    'alpha-write-off': [80,64,64,64], 'occluded': [16,32,48,64],
    'depth-write-off': [32,64,32,255], 'depth-write-on-control': [64,32,16,255],
    'drawn-occluder': [64,32,16,255],
}
products = [srgb_decode(a)*srgb_decode(b) for a,b in zip([128,128,64], [128,64,64])]
expected['srgb-product'] = [srgb_encode(value) for value in products]+[255]
# The occluded clear case measures the conversion directly: the normalized
# Clear components become sRGB-encoded target bytes on this tested device.
expected['srgb-clear-occluded'] = [srgb_encode(value/255) for value in [16,32,48]]+[64]
expected['srgb-additive'] = [srgb_encode(value+background/255)
                           for value,background in zip(products,[16,32,48])]+[255]
# A second draw samples a different quadrant. Decode the first draw's stored
# result before adding: this distinguishes actual sRGB destination blending
# from simply adding gamma-encoded byte values.
second_products = [srgb_decode(a)*srgb_decode(b) for a,b in zip([128,128,64],[64,128,128])]
expected['srgb-layered'] = [srgb_encode(srgb_decode(round(stored))+value)
                          for stored,value in zip(expected['srgb-additive'][:3],second_products)]+[255]
expected['srgb-layered-unorm-control'] = [round(background+first*255)+second*255
    for background,first,second in zip([16,32,48],products,second_products)]+[255]
rows = list(csv.DictReader((directory/'captures/pixels.csv').open()))
require({row['case'] for row in rows} == set(expected), 'Missing or duplicate GPU cases')
require(len(rows) == len(expected), 'Incorrect case count')
records = []
for row in rows:
    name = row['case']
    path = directory/'captures'/(name+'.rgba')
    rgba = path.read_bytes()
    require(len(rgba) == 64*64*4, 'Capture size differs')
    center = list(rgba[(32*64+32)*4:(32*64+32)*4+4])
    require(center == [int(row[c]) for c in ['r','g','b','a']], 'CSV differs from GPU readback')
    tolerance = 2 if name.startswith('srgb-') else 1
    errors = [abs(value-target) for value,target in zip(center,expected[name])]
    require(max(errors) <= tolerance, 'Independent expected value mismatch: '+name)
    # Inspect a full interior tile; avoid relying on just a single center sample.
    for y in range(16,48):
        for x in range(16,48):
            require(list(rgba[(y*64+x)*4:(y*64+x)*4+4]) == center,
                    'Unexpected interior variation: '+name)
    records.append(dict(case=name, center_rgba=center, expected_rgba=expected[name],
                        tolerance=tolerance, maximum_channel_error=max(errors),
                        interior_pixels_checked=1024, rgba_sha256=digest(path)))
for name,value in programs.items():
    require(digest(SHADERS/name) == value, 'Extracted program changed')
receipt = dict(status='PASS_D3D9_GPU_REFERENCE', executable=str(EXE), executable_sha256=digest(EXE),
               architecture='AMD64 PE32+', shader_directory=str(SHADERS), shader_sha256=programs,
               cases=records, runtime_log='runtime.log',
               limitations=['Explicit synthetic states, not a live SFM material snapshot.',
                            'Depth comparison LESS is chosen for the diagnostic, not recovered as the SFM default.',
                            'sRGB clear conversion measured on the tested device; broader device coverage remains incomplete.',
                            'Vulkan comparison and editor integration are not completed.'])
(directory/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
print('PASS: AMD64; 14 GPU cases; 14,336 interior pixels; independent RGB/alpha expectations')
print(directory/'results.json')
