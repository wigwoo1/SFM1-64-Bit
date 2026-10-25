"""Extract a hash-pinned launcher service list from bounded constant stack writes.

LLVM reads the original as data; original code is never loaded or executed. This
extractor supports only the MOV forms present before one known AddSystems call.
It is not a general x86 interpreter and does not infer runtime initialization order.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess

from pe_image import Image


def extract_list(disassembly, image):
    registers, stack = {}, {}
    for line in disassembly.splitlines():
        instruction = re.match(r'^([0-9a-f]+):.*?\bmov\s+(.+)$', line, re.I)
        if not instruction:
            continue
        address, operands = int(instruction[1], 16), instruction[2].strip()
        register = re.fullmatch(r'(eax|ecx|edx|esi|edi), (.+)', operands)
        if register:
            destination, value = register.groups()
            registers[destination] = int(value, 16) if re.fullmatch(r'0x[0-9a-f]+', value) else None
            continue
        store = re.fullmatch(r'dword ptr \[ebp - (0x[0-9a-f]+)\], (eax|ecx|0x[0-9a-f]+)', operands)
        if not store:
            continue
        offset, value = int(store[1], 16), store[2]
        if not 8 <= offset <= 0x94:
            continue
        number = registers.get(value) if value in registers else int(value, 16)
        if number is None:
            raise ValueError('Unknown register value at ' + hex(address))
        stack[offset] = dict(value=number, instruction=hex(address))
    rows = []
    for offset in range(0x94, 7, -8):
        module, interface = stack[offset], stack[offset - 4]
        module_name, interface_name = image.string(module['value']), image.string(interface['value'])
        if not module_name:
            if interface_name or offset != 0xC:
                raise ValueError('Unexpected list terminator')
            return rows
        rows.append(dict(index=len(rows), module=module_name, interface=interface_name,
                         module_write=module['instruction'], interface_write=interface['instruction']))
    raise ValueError('Expected terminator not found')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('root', type=Path)
    ap.add_argument('output', type=Path)
    ap.add_argument('--llvm', required=True, type=Path)
    args = ap.parse_args()
    root, output = args.root.resolve(strict=True), args.output.resolve()
    if output == root or root in output.parents:
        ap.error('Output must be outside the read-only installation')
    path = root / 'bin/launcher.dll'
    image = Image(path)
    digest = hashlib.sha256(image.b).hexdigest()
    if digest != '7eed90e02bd89d30ef06c314469a53eaa50281dc994e70912da7ba44b29be85e':
        raise ValueError('Launcher hash changed; recorded addresses are inapplicable')
    command = [str(args.llvm), '--disassemble', '--x86-asm-syntax=intel',
               '--start-address=0x10002aa0', '--stop-address=0x10002bad', str(path)]
    result = subprocess.run(command, capture_output=True, text=True, check=True)
    rows = extract_list(result.stdout, image)
    checks = []

    def check(label, actual, expected):
        if actual != expected:
            raise ValueError(label + ': unexpected ' + repr(actual))
        checks.append(label)

    check('registration list length', len(rows), 17)
    check('material system position', (rows[3]['module'], rows[3]['interface']),
          ('materialsystem.dll', 'VMaterialSystem080'))
    check('engine launcher API position', (rows[16]['module'], rows[16]['interface']),
          ('engine.dll', 'VENGINE_LAUNCHER_API_VERSION004'))
    for site, target in ((0x10002BAD, 0x100095A0), (0x10002A68, 0x10008C70),
                         (0x100095B4, 0x10008AD0), (0x100095C0, 0x10009460),
                         (0x10009808, 0x10009600), (0x1000981A, 0x100092B0),
                         (0x10009840, 0x10008D90), (0x10009988, 0x10009460),
                         (0x100099A3, 0x10009460), (0x1000974A, 0x10008AD0),
                         (0x10009756, 0x10009460), (0x10004F76, 0x10008E70),
                         (0x10004F8A, 0x1000E860), (0x10004F93, 0x1000DBF0),
                         (0x10004FA1, 0x10010500), (0x10004FAF, 0x100105E0),
                         (0x10005063, 0x100024E0), (0x100050A5, 0x10002650),
                         (0x1000511B, 0x10004BC0), (0x10009639, 0x10008050),
                         (0x10009644, 0x10008FF0)):
        offset = image.offset(site)
        check('CALL opcode at ' + hex(site), image.b[offset], 0xE8)
        check('CALL target at ' + hex(site), site + 5 + struct.unpack_from('<i', image.b, offset + 1)[0], target)
    for location, expected in ((0x10002D07, 'shaderapidx9.dll'), (0x10002D2F, 'shaderapiempty.dll'),
                               (0x10002D1F, '-noshaderapi'), (0x10002D42, '-shaderapi'),
                               (0x10002A62, 'VFileSystem017'),
                               (0x10009981, 'VEngineCvar007'), (0x1000999C, 'VFileSystem017')):
        check('string operand at ' + hex(location), image.string(image.word(location)), expected)
    check('material backend vtable byte offset', image.b[image.offset(0x10002D5F)], 0x24)
    check('AddSystems pair increment', image.b[image.offset(0x100095CE)], 8)
    check('Connect-loop factory callback', image.word(0x100092CB), 0x10008E40)
    check('Init-loop vtable offset', image.b[image.offset(0x10008DAB)], 0x0C)
    check('dependency-query vtable offset', image.b[image.offset(0x100096C3)], 0x14)
    descriptor = image.word(image.word(0x10023CE4 - 4) + 12)
    check('launcher group RTTI', image.string(descriptor + 8), '.?AVCSourceAppSystemGroup@@')
    for entry, slot, name in ((0x10029298, 0x1002B99C, 'VFileSystem017'),
                              (0x100292B0, 0x1002B9A8, 'VMaterialSystem080')):
        check('PreInit interface name at ' + hex(entry), image.string(image.word(entry)), name)
        check('PreInit interface output at ' + hex(entry), image.word(entry + 4), slot)
    check('filesystem PreInit null gate', image.word(0x10004FB9), 0x1002B99C)
    check('material PreInit null gate', image.word(0x10004FC6), 0x1002B9A8)
    check('edit-mode flag', image.string(image.word(0x10002A98)), '-edit')
    check('engine startup-data receiver slot', image.b[image.offset(0x10005183)], 0x20)
    check('system permutation store opcode', image.b[image.offset(0x10009234)], 0x89)
    check('system map index repair opcode', image.b[image.offset(0x10009274)], 0x89)
    check('circular dependency diagnostic', image.string(0x100243A8),
          'Encountered a circular dependency with library %s!\n')
    output.parent.mkdir(parents=True, exist_ok=True)
    data = dict(passed=True, sha256=digest, module='bin/launcher.dll', checks=checks, check_count=len(checks),
                ordinary_registration_list=rows, llvm_command=command,
                preinit_and_ordering=dict(preinit='0x10004F60',
                    required_interfaces=['VFileSystem017', 'VMaterialSystem080'],
                    dependency_closure_helper='0x10008050', reorder_helper='0x10008FF0',
                    permutation_array_write='0x10009234', index_map_repair='0x10009274',
                    engine_startup_data_call='0x1000518B',
                    limit='Landmarks corroborate manual disassembly; exact graph behavior, live order and runtime state are not reconstructed by this extractor.'),
                limits='Base registration list and phase-call landmarks; final per-service order, full dependency closure and runtime success are unverified.')
    output.write_text(json.dumps(data, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(passed=True, checks=len(checks), registration_pairs=len(rows))))
    for row in rows:
        print(row['index'], row['module'], row['interface'])


if __name__ == '__main__':
    main()
