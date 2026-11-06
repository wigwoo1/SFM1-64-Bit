"""Independent float blending/alpha oracle and actual-editor transparency checks."""
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

ROOT = Path(__file__).resolve().parent.parent
EDITOR_RECEIPT = ROOT / 'outputs/viewport-output-editor-gq_u0ne9/verification-xy8cfd11/results.json'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def half(value):
    return struct.unpack('<e', struct.pack('<e', value))[0]


def srgb(value):
    value = min(1., max(0., value))
    return 12.92 * value if value <= .0031308 else 1.055 * value ** (1. / 2.4) - .055


def byte(value):
    return math.floor(min(1., max(0., value)) * 255 + .5)


def rounded(color):
    return tuple(half(f32(value)) for value in color)


def over(destination, source):
    source = tuple(map(f32, source))
    alpha = source[3]
    return tuple(half(source[channel] * alpha + destination[channel] * (1 - alpha))
                 for channel in range(3)) + (half(alpha + destination[3] * (1 - alpha)),)


def reference(row, x):
    alpha = f32(x / 255.)
    if row == 0: return rounded((.18, .5, .003, alpha))
    if row == 1: return over(rounded((.1, .6, .001, .25)), (.8, .2, .5, alpha))
    if row == 2:
        first = over(rounded((.125, .5, .9, .125)), (3, -.25, .5, .5))
        return over(first, (.001, .18, .25, alpha))
    if row == 3:
        background = rounded((.25, .5, 2, .15))
        added = tuple(half(background[c] + f32(v)) for c, v in enumerate((1.5, .25, -.125))) + background[3:]
        return over(added, (.001, .18, .25, alpha))
    if row == 4: return rounded((x / 255., ((x + 47) % 256) / 255., ((x + 137) % 256) / 255., ((x + 71) % 256) / 255.))
    if row == 5: return rounded((.0031308, .003, .0032, alpha))
    raise ValueError(row)


def half_bounds(value):
    """Adjacent binary16 values bracketing a finite exact result, no fitted epsilon."""
    nearest = half(value)
    bits = struct.unpack('<H', struct.pack('<e', nearest))[0]
    def adjacent(up):
        if nearest == 0: return 2**-24 if up else -2**-24
        step = 1 if (nearest > 0) == up else -1
        return struct.unpack('<e', struct.pack('<H', bits + step))[0]
    return (nearest if nearest <= value else adjacent(False),
            nearest if nearest >= value else adjacent(True))


def qinterval(bounds):
    return half_bounds(bounds[0])[0], half_bounds(bounds[1])[1]


def add(left, right):
    return qinterval((left[0]+right[0], left[1]+right[1]))


def multiply(left, right):
    products = [a*b for a in left for b in right]
    return qinterval((min(products), max(products)))


def stored(color):
    return tuple(half_bounds(f32(value)) for value in color)


def bounded_over(destination, source):
    # Vulkan permits destination-format blend precision and either neighboring
    # float conversion. Propagate those choices through each arithmetic step.
    source = stored(source)
    alpha = source[3]
    complement = qinterval((1-alpha[1], 1-alpha[0]))
    return tuple(add(multiply(source[c], alpha), multiply(destination[c], complement))
                 for c in range(3)) + (add(alpha, multiply(destination[3], complement)),)


def bounded_reference(row, x):
    alpha = f32(x/255.)
    if row == 0: return stored((.18,.5,.003,alpha))
    if row == 1: return bounded_over(stored((.1,.6,.001,.25)),(.8,.2,.5,alpha))
    if row == 2:
        first = bounded_over(stored((.125,.5,.9,.125)),(3,-.25,.5,.5))
        return bounded_over(first,(.001,.18,.25,alpha))
    if row == 3:
        first = stored((.25,.5,2,.15))
        first = tuple(add(first[c],stored((1.5,.25,-.125))[c]) for c in range(3)) + first[3:]
        return bounded_over(first,(.001,.18,.25,alpha))
    if row == 4: return stored((x/255.,((x+47)%256)/255.,((x+137)%256)/255.,((x+71)%256)/255.))
    if row == 5: return stored((.0031308,.003,.0032,alpha))
    raise ValueError(row)


def oracle(directory):
    encoded = (directory / 'encoded.rgba').read_bytes()
    linear = (directory / 'linear.rgba16f').read_bytes()
    need(len(encoded) == 256 * 6 * 4 and len(linear) == 256 * 6 * 8, 'Full GPU capture extents')
    maximum_float_interval = 0.
    maximum_rgb_error = 0
    maximum_alpha_error = 0
    negative = dict(alpha_gamma=0, encoded_space_blend=0, early_clamp=0, premultiplied=0, double_encoding=0)
    for row in range(6):
        for x in range(256):
            index = row * 256 + x
            actual = struct.unpack_from('<eeee', linear, index * 8)
            expected = bounded_reference(row, x)
            output = tuple(encoded[index * 4:index * 4 + 4])
            for channel, (value, bounds) in enumerate(zip(actual, expected)):
                need(bounds[0] <= value <= bounds[1], f'Float blend row {row} x {x} channel {channel}: {value} outside {bounds}')
                maximum_float_interval = max(maximum_float_interval, bounds[1]-bounds[0])
            wanted_rgb = tuple(byte(srgb(value)) for value in actual[:3])
            rgb_error = max(abs(a-b) for a,b in zip(output[:3], wanted_rgb))
            alpha_error = abs(output[3] - byte(actual[3]))
            need(rgb_error <= 1 and alpha_error <= 1, 'Output copy RGB or linear alpha mismatch')
            maximum_rgb_error = max(maximum_rgb_error, rgb_error)
            maximum_alpha_error = max(maximum_alpha_error, alpha_error)
            if row in [0, 5]:
                need(output[3] == x, 'Pass-through alpha ramp was changed')
                negative['alpha_gamma'] += abs(output[3] - byte(srgb(actual[3]))) > 2
            negative['double_encoding'] += max(abs(output[c] - byte(srgb(output[c] / 255.))) for c in range(3)) > 2
            if row == 1:
                a = f32(x / 255.)
                wrong = tuple(byte(srgb(s) * a + srgb(d) * (1-a)) for s,d in zip((.8,.2,.5),(.1,.6,.001)))
                negative['encoded_space_blend'] += max(abs(v-w) for v,w in zip(output, wrong)) > 2
                wrong = tuple(byte(srgb(s + d * (1-a))) for s,d in zip((.8,.2,.5),(.1,.6,.001)))
                negative['premultiplied'] += max(abs(v-w) for v,w in zip(output, wrong)) > 2
            if row == 2:
                a = f32(x / 255.)
                # Wrong policy clamps the first blend before the second layer.
                first = over(rounded((.125,.5,.9,.125)), (3,-.25,.5,.5))
                wrong = tuple(byte(srgb(s*a + min(1.,max(0.,d))*(1-a))) for s,d in zip((.001,.18,.25),first))
                negative['early_clamp'] += max(abs(v-w) for v,w in zip(output,wrong)) > 2
    need(negative['alpha_gamma'] > 450 and negative['double_encoding'] > 1000 and
         all(negative[name] > 150 for name in ['encoded_space_blend','premultiplied','early_clamp']), 'Conversion-error controls distinguishability')
    return dict(float_channel_checks=6144, output_channel_checks=6144, exact_alpha_ramp_checks=512,
                float_oracle='binary16 interval propagation, not a fixed epsilon or measured-value fit',
                max_float_interval_width=maximum_float_interval, max_rgb_byte_error=maximum_rgb_error,
                max_alpha_byte_error=maximum_alpha_error, distinguishable_wrong_policy_pixels=negative)


def main():
    need(len(sys.argv) == 2, 'Supply the workspace output directory containing before.json')
    parent = Path(sys.argv[1]).resolve()
    need(parent.is_relative_to(ROOT / 'outputs'), 'Workspace outputs only')
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=parent))
    before = json.loads((parent / 'before.json').read_text())
    editor_record = json.loads(EDITOR_RECEIPT.read_text())
    editor = Path(editor_record['executables']['candidate']['path'])
    need(sha(editor) == editor_record['executables']['candidate']['sha256'], 'Preserved editor changed')
    for name, digest in editor_record['changed_source_sha256'].items():
        if name != 'README.md': need(sha(ROOT / name) == digest, 'Tested editor dependency changed ' + name)
    for name, digest in editor_record['new_source_sha256'].items():
        if name != 'verify_editor_viewport_output_candidate.py': need(sha(ROOT / 'work' / name) == digest, 'Tested helper/shader source changed')
    clean = {key: value for key,value in os.environ.items() if not key.startswith('SFM64_')}
    result = dict(status='IN PROGRESS', time_utc=datetime.now(timezone.utc).isoformat(), commands=[],
                  editor=dict(path=str(editor), sha256=sha(editor), rebuilt=False, reason='No editor source dependency changed'),
                  previous_editor_receipt=str(EDITOR_RECEIPT), previous_editor_receipt_sha256=sha(EDITOR_RECEIPT))

    def execute(arguments, label, env=None):
        process = subprocess.run(list(map(str, arguments)), cwd=ROOT, env=env or clean, capture_output=True,
                                 text=True, timeout=90, creationflags=subprocess.CREATE_NO_WINDOW)
        text = process.stdout + process.stderr
        (run / (label + '.log')).write_text(text)
        result['commands'].append(dict(arguments=list(map(str, arguments)), exit_code=process.returncode))
        need(process.returncode == 0, label + ' failed ' + text[-2000:])
        return text

    try:
        for name, digest in editor_record['shader_sha256'].items():
            need(sha(editor.parent / 'shaders' / name) == digest, 'Preserved editor shader changed')
        build = run / 'probe'
        execute(['cmd.exe','/d','/c',ROOT/'work/build_viewport_output_blending_x64.cmd',build], 'build')
        binary = build / 'viewport_output_blending_x64.exe'
        raw = binary.read_bytes(); offset = struct.unpack_from('<I',raw,60)[0]
        need(raw[offset:offset+4] == b'PE\0\0' and struct.unpack_from('<H',raw,offset+4)[0] == 0x8664 and
             struct.unpack_from('<H',raw,offset+24)[0] == 0x20b, 'Probe AMD64 PE32+')
        warnings = re.findall(r'warning (C\d+): ([^\r\n]+)', (build/'build.log').read_text())
        need(len(warnings) == 3 and all(code == 'C4996' and "'getenv'" in message for code,message in warnings), 'New probe compiler warnings')
        need(not (build/'shader-build.log').read_text().strip(), 'Test shader compiler diagnostic')
        result['probe'] = dict(path=str(binary),sha256=sha(binary),compiler_warnings=warnings)
        gpu = run / 'gpu'
        text = execute([binary,editor.parent/'shaders',build/'blend-probe.spv',gpu], 'gpu')
        need('PASS shared_editor_output_copy=1' in text and 'tracked_memory_released=1' in text, 'GPU probe acceptance')
        result['gpu_oracle'] = oracle(gpu)
        print('PASS independent shared-helper float blending, RGB and alpha oracle', flush=True)
        # Generated sessions/assets only. Declare transparency explicitly in VMT
        # and VTF metadata; no native installation or assets are needed here.
        original = EDITOR_RECEIPT.parent / 'owned'
        fixtures = run / 'fixtures'
        fixtures.mkdir()
        captures = {}
        for layers, alpha in [(layers,alpha) for layers in [1,2] for alpha in [255,0,64,128]]:
            label = 'layers-' + str(layers) + '-alpha-' + str(alpha)
            own = fixtures / label
            shutil.copytree(original, own)
            vtf = own / 'materials/owned/pixel.vtf'
            data = bytearray(vtf.read_bytes())
            struct.pack_into('<I', data, 20, struct.unpack_from('<I', data, 20)[0] | 0x2000)
            for at in range(88 + 3, len(data), 4): data[at] = alpha
            vtf.write_bytes(data)
            for name in ['mat','other']:
                (own/'materials'/(name+'.vmt')).write_text('LightmappedGeneric { $basetexture owned/pixel $translucent 1 }\n')
            # The fallback world route interpolates preview lighting at vertices.
            # Author a constant white HDR lightmap so this is a controlled lit
            # translucency fixture rather than relying on unlit shader support.
            bsp = own/'maps/owned.bsp'
            lightmap = bytearray(bsp.read_bytes())
            offset, size = struct.unpack_from('<II',lightmap,8+53*16)
            need(size and size % 4 == 0, 'Owned HDR lightmap framing')
            lightmap[offset:offset+size] = bytes([255,255,255,0])*(size//4)
            # Use explicitly authored identical quads, not the inherited mix of
            # overlapping quads and a separate triangle from the lighting test.
            faces = struct.unpack_from('<I',lightmap,8+58*16)[0]
            lightmap[faces+2*56:faces+3*56] = lightmap[faces+56:faces+2*56]
            model = struct.unpack_from('<I',lightmap,8+14*16)[0]
            struct.pack_into('<II',lightmap,model+40,1,layers)
            bsp.write_bytes(lightmap)
            hashes = {str(p.relative_to(own)):sha(p) for p in own.rglob('*') if p.is_file()}
            out = run / ('editor-' + label); out.mkdir()
            env = dict(clean, SFM64_EXPERIMENTAL_HDR2_PROFILE=str(own/'113-nearest-1.profile'),
                       SFM64_DIAGNOSTIC_CAPTURE_FRAMES='1',SFM64_DIAGNOSTIC_OUTPUT_DIRECTORY=str(out))
            text = execute([editor,'--control-test','--session',own/'owned.dmx','--content-directory',own], 'editor-'+label,env)
            need('model_gpu_presentation_passed=1 frames=5' in text and 'hdr2_output_copy=1' in text, 'Actual editor translucency route')
            need(text.count('editor_hdr2_draw_fence_completed=') == 5, 'Actual editor draw fences')
            files = {p.name:p.read_bytes() for p in out.glob('*.rgba')}
            need(files, 'Actual editor captures absent')
            clear = tuple(byte(srgb(half(value))) for value in [.05,.05,.08]) + (255,)
            if alpha == 255:
                masks = {name: [pixel == (255,255,255,255) for pixel in zip(*[iter(data)]*4)] for name,data in files.items()}
                need(all(any(mask) and not all(mask) for mask in masks.values()), 'Opaque fixture coverage')
            metrics = {}
            for name,data in files.items():
                pixels = list(zip(*[iter(data)]*4)); need(len(pixels) == 512*512,'Actual editor fixture extent')
                a = half(alpha/255.)
                light = list(map(half,[.05,.05,.08]))
                for _ in range(layers): light = [half(a + value*(1-a)) for value in light]
                want = tuple(byte(srgb(value)) for value in light) + (255,)
                maximum = 0
                for pixel,covered in zip(pixels,masks[name]):
                    expected = want if covered else clear
                    error = max(abs(v-w) for v,w in zip(pixel,expected)); maximum=max(maximum,error)
                    need(error <= 1, 'Actual editor '+label+' got '+str(pixel)+' expected '+str(expected))
                metrics[name] = dict(pixels=len(pixels),max_byte_error=maximum,sha256=sha(out/name))
            need(all(sha(own/name)==digest for name,digest in hashes.items()),'Owned session/asset modified')
            captures[label] = metrics
            print('PASS actual editor translucent '+label,flush=True)
        need(all(sha(ROOT/name)==digest for name,digest in before['editor_source_sha256'].items()),'Production editor dependency changed')
        result.update(status='VERIFIED shared output helper alpha/linear blending and controlled actual editor translucency; native material parity incomplete',
                      actual_editor=captures,editor_source_sha256=before['editor_source_sha256'],
                      source_sha256={name:sha(ROOT/'work'/name) for name in [
                          'probe_viewport_output_blending_x64.cpp','viewport_output_blend_probe.hlsl',
                          'build_viewport_output_blending_x64.cmd',Path(__file__).name]},
                      limits=['All synthetic input; original SFM and F: unused.',
                              'One tested GPU, Vulkan validation layer not enabled.',
                              'The real output helper is shared with the editor. Controlled blend pipelines mirror editor blend factors but do not prove all native material operations.',
                              'Actual editor tests cover generated white-lit LightmappedGeneric translucent faces in one and two overlapping layers over an opaque clear; native alpha targets, ordering, sprites and particle parity remain incomplete.'])
    except Exception as error:
        result.update(status='PARTIALLY VERIFIED',failure=repr(error));raise
    finally:
        receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n')
        print('receipt='+str(receipt),flush=True)
    print(json.dumps({'status':result['status'],'gpu_oracle':result['gpu_oracle'],
                      'actual_editor_pixels':sum(m['pixels'] for v in captures.values() for m in v.values())},indent=2))


if __name__ == '__main__':
    main()
