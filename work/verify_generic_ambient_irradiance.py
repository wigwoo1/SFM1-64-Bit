"""Reference arithmetic from actual VS20 tokens, not a hand-written cube sum.

Backward component slicing excludes normal transformation and unrelated lights.
MAD is evaluated as separate float32 multiply/add: a CPU reference convention,
not a claim about native GPU rounding, shader selection or complete rendering.
"""
from pathlib import Path
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import math
import random
import struct
import subprocess
import tempfile

from inspect_generic_ambient_shader import ROOT, OUT, SOURCE, extract


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def f32(value):
    return struct.unpack('<f', struct.pack('<f', value))[0]


def reg(token):
    return ((token >> 28) & 7) | ((token >> 8) & 0x18), token & 0x7ff


def source_operand(token, relative=None):
    return dict(reg=reg(token), swizzle=[(token >> (16+2*i)) & 3 for i in range(4)],
                modifier=(token >> 24) & 15, relative=relative)


def instructions(shader):
    words = struct.unpack('<'+'I'*(len(shader)//4), shader)
    require(words[0] == 0xfffe0200, 'VS20 required')
    result, at = [], 1
    while at < len(words):
        start, word = at, words[at]
        opcode = word & 0xffff
        if opcode == 0xffff:
            require(at == len(words)-1, 'Trailing shader words')
            return result
        if opcode == 0xfffe:
            at += 1+((word >> 16) & 0x7fff)
            require(at <= len(words), 'Shader comment bounds')
            continue
        length = (word >> 24) & 15
        require(length > 0 and at+length < len(words), 'Shader instruction bounds')
        operands = words[at+1:at+1+length]
        item = dict(offset_words=start, opcode=opcode, token=word, raw_operands=operands)
        # Decode arithmetic only. Other instructions may be skipped by the
        # slice, but cause refusal if they write a value the result needs.
        item.update(dest=reg(operands[0]), mask=(operands[0] >> 16) & 15,
                    dest_modifier=(operands[0] >> 20) & 15)
        if opcode in (1, 4, 5, 12, 36, 46):
            sources, cursor = [], 1
            while cursor < len(operands):
                token = operands[cursor]
                cursor += 1
                relative = None
                if token & 0x2000:
                    require(cursor < len(operands), 'Relative operand bounds')
                    relative = source_operand(operands[cursor])
                    cursor += 1
                sources.append(source_operand(token, relative))
            item['sources'] = sources
        result.append(item)
        at += 1+length
    raise RuntimeError('Missing shader END')


def ambient_slice(shader):
    parsed = instructions(shader)
    endings = [i for i, op in enumerate(parsed) if op['opcode'] == 4 and
               any(s['reg'] == (2,25) and s['relative'] for s in op.get('sources', []))]
    require(len(endings) == 1, 'One ambient Z completion required')
    end = endings[0]
    start = max(i for i in range(end) if parsed[i]['opcode'] == 36)
    normal = parsed[start]['dest']
    needed = {(parsed[end]['dest'], channel) for channel in range(3)}
    chosen = []
    for op in reversed(parsed[start+1:end+1]):
        mask = [c for c in range(4) if op['mask'] & (1 << c) and (op['dest'], c) in needed]
        if not mask:
            continue
        require(op['opcode'] in (1,4,5,12,46) and not op['dest_modifier'] and not op['token'] & 0x10000000,
                'Unsupported operation in ambient dependency slice')
        require(len(op['sources']) == {1:1,4:3,5:2,12:2,46:1}[op['opcode']], 'Arithmetic source count')
        for c in mask:
            needed.remove((op['dest'],c))
        for s in op['sources']:
            require(s['modifier'] in (0,1), 'Unsupported source modifier')
            if s['reg'][0] != 2:
                for c in mask:
                    needed.add((s['reg'], s['swizzle'][c]))
            if s['relative']:
                a = s['relative']
                require(a['reg'] == (3,0) and not a['modifier'] and not a['relative'], 'Unexpected relative addressing')
                needed.add((a['reg'], a['swizzle'][0]))
        chosen.append(dict(op, used_components=mask))
    chosen.reverse()
    require(needed == {(normal,c) for c in range(3)}, 'Ambient expression has an unresolved input')
    require(Counter(op['opcode'] for op in chosen) == {12:1,46:1,5:2,4:2}, 'Unexpected ambient expression shape')
    constants = sorted({s['reg'][1] for op in chosen for s in op['sources'] if s['reg'][0] == 2})
    require(constants == [0,21,23,25], 'Unexpected ambient input constants')
    return normal, chosen, parsed[end]['dest']


def execute(normal, cube, sliced):
    normal_reg, operations, output = sliced
    registers = {(normal_reg,c):normal[c] for c in range(3)}

    def read(s, lane):
        register, index = s['reg']
        component = s['swizzle'][lane]
        if s['relative']:
            a = s['relative']
            index += int(registers[(a['reg'],a['swizzle'][0])])
        if register == 2:
            if index == 0:
                require(component == 0, 'Only explicitly supplied c0.x=0 is available')
                value = 0.0
            else:
                require(21 <= index <= 26 and component < 3, 'Ambient constant outside cube')
                value = cube[index-21][component]
        else:
            value = registers[((register,index),component)]
        return -value if s['modifier'] else value

    for op in operations:
        values = {}
        for c in op['used_components']:
            inputs = [read(s,c) for s in op['sources']]
            if op['opcode'] == 12:
                value = float(inputs[0] < inputs[1])
            elif op['opcode'] == 46:
                require(inputs[0] in (0.0,1.0), 'MOVA reference domain')
                value = float(round(inputs[0]))
            elif op['opcode'] == 5:
                value = f32(inputs[0]*inputs[1])
            elif op['opcode'] == 4:
                value = f32(f32(inputs[0]*inputs[1])+inputs[2])
            else:
                value = inputs[0]
            values[(op['dest'],c)] = value
        registers.update(values)
    return [registers[(output,c)] for c in range(3)]


def main():
    extraction = extract()
    run = Path(tempfile.mkdtemp(prefix='verification-', dir=OUT))
    before = {p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in (
        'work/generic_ambient_irradiance_candidate.h', 'work/test_generic_ambient_irradiance.cpp',
        'work/inspect_generic_ambient_shader.py', 'work/build_generic_ambient_irradiance_x64.cmd',
        'work/verify_generic_ambient_irradiance.py', 'work/probe_vulkan_win32_surface_x64.cpp',
        'work/sfm64-material-alpha-candidate.hlsl')}
    axes = [(1,0,0),(-1,0,0),(0,1,0),(0,-1,0),(0,0,1),(0,0,-1)]
    normals = axes + [(0,0,0),(-0.0,0,-0.0),(2,-3,4),(0.5,-0.5,0.5)]
    for x in (-1,1):
        for y in (-1,1):
            for z in (-1,1):
                normals.append(tuple(f32(v/math.sqrt(3)) for v in (x,y,z)))
    rng = random.Random(2026102407)
    for _ in range(4096):
        n = [rng.uniform(-1,1) for _ in range(3)]
        length = math.sqrt(sum(x*x for x in n))
        normals.append(tuple(f32(x/length) for x in n))
    fixtures = []
    for normal in normals:
        fixtures.append((normal,[[f32(rng.uniform(-2,32)) for _ in range(3)] for _ in range(6)]))
    for face in range(6):
        for channel in range(3):
            cube = [[0.0]*3 for _ in range(6)]
            cube[face][channel] = 1.0
            for normal in normals[:18]:
                fixtures.append((normal,cube))
    cube = [[f32(0.25*(face+1)),f32(0.5*(face+1)),float(face+1)] for face in range(6)]
    for normal in axes:
        require(execute(normal,cube,ambient_slice((OUT/extraction['programs'][0]['bytecode']).read_bytes())) == cube[axes.index(normal)],
                'Independent one-axis face selection')
    rows, programs = [], []
    first_program_results = None
    for program in extraction['programs']:
        shader = (OUT/program['bytecode']).read_bytes()
        sliced = ambient_slice(shader)
        current_results = []
        for normal, cube in fixtures:
            result = execute(normal,cube,sliced)
            current_results.append(struct.pack('<3f',*result))
            rows.append(struct.pack('<24f',*normal,*[c for face in cube for c in face],*result))
        if first_program_results is None:
            first_program_results = current_results
        else:
            require(current_results == first_program_results, 'Ambient expression differs between shader variants')
        programs.append(dict(dynamic=program['dynamic'], bytecode_sha256=program['bytecode_sha256'],
                             normal_register=sliced[0], output_register=sliced[2],
                             expression_instruction_word_offsets=[op['offset_words'] for op in sliced[1]],
                             opcode_counts=dict(Counter(op['opcode'] for op in sliced[1])), cases=len(fixtures)))
    fixture = run/'reference.bin'
    fixture.write_bytes(b''.join(rows))
    executable = OUT/'ambient_irradiance_x64.exe'
    raw = executable.read_bytes()
    pe = struct.unpack_from('<I',raw,60)[0]
    require(raw[pe:pe+4] == b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0] == 0x8664 and
            struct.unpack_from('<H',raw,pe+24)[0] == 0x20b, 'Native AMD64/PE32+ required')
    process = subprocess.run([str(executable),str(fixture)], capture_output=True,text=True,timeout=30)
    (run/'test.log').write_text(process.stdout+process.stderr)
    require(process.returncode == 0, process.stdout+process.stderr)
    require('analytical=9 guard_rejections=64 bytecode_reference_cases='+str(len(rows))+' x64=1' in process.stdout, 'Coverage mismatch')
    require(hashlib.sha256(SOURCE.read_bytes()).hexdigest() == extraction['source_sha256'], 'Original cache changed')
    require(all(hashlib.sha256((ROOT/p).read_bytes()).hexdigest() == h for p,h in before.items()), 'Source changed during verification')
    build_log = (OUT/'build.log').read_text()
    require('warning ' not in build_log.lower() and 'error ' not in build_log.lower(), 'Build diagnostics require review')
    receipt = dict(status='PASS', checked_at_utc=datetime.now(timezone.utc).isoformat(),
        cases=len(rows), fixtures_per_program=len(fixtures), programs=programs,
        analytical=9, guard_rejections=64, cross_variant_exact_comparisons=3*len(fixtures), test_output=process.stdout.strip(),
        executable=str(executable), executable_sha256=hashlib.sha256(raw).hexdigest(),
        fixture_sha256=hashlib.sha256(fixture.read_bytes()).hexdigest(), sources_sha256=before,
        shader_cache_sha256=extraction['source_sha256'], original_cache_unchanged=True,
        build_log_sha256=hashlib.sha256((OUT/'build.log').read_bytes()).hexdigest(),
        limitations=[
            'Interpreter evaluates only the dependency-sliced six-instruction ambient expression from four explicit generic VS20 programs.',
            'Normal transform/normalization, active combo selection, lighting point, cube sampling, HDR/LDR, local/static lights, textures, gamma and complete shader execution are excluded.',
            'c0.x=0 and normalized world normals are explicit reference inputs, not measured native state. Non-unit/zero inputs test expression behavior only.',
            'Exact CPU float32 results use non-fused MAD rounding; actual native GPU precision/behavior remains UNVERIFIED.',
            'No SFM process, live probing, computer use, D3D9 device or GPU used; no editor integration/rebuild or visual parity claim.',
        ])
    (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(status=receipt['status'],cases=len(rows),programs=len(programs),analytical=9,
                          guard_rejections=64,executable_sha256=receipt['executable_sha256'],receipt=str(run/'results.json'))))


if __name__ == '__main__':
    main()
