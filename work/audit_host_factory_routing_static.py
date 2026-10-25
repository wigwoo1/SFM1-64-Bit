"""Read-only, hash-pinned extraction of IFM service bindings and factory landmarks.

This is static evidence for two original x86 images, not an implementation of their ABI.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from pe_image import Image


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('root', type=Path)
    ap.add_argument('output', type=Path)
    args = ap.parse_args()
    root, output = args.root.resolve(strict=True), args.output.resolve()
    if output == root or root in output.parents:
        ap.error('Output must be outside the read-only installation')
    engine = Image(root / 'bin/engine.dll')
    ifm = Image(root / 'bin/tools/ifm.dll')
    checks = []

    def check(label, actual, expected):
        if actual != expected:
            raise ValueError(label + ': actual=' + repr(actual) + ', expected=' + repr(expected))
        checks.append(label)

    hashes = {'engine.dll': hashlib.sha256(engine.b).hexdigest(),
              'ifm.dll': hashlib.sha256(ifm.b).hexdigest()}
    check('engine hash pin', hashes['engine.dll'], 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f')
    check('IFM hash pin', hashes['ifm.dll'], '8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4')
    check('factory getter MOV-immediate opcode', engine.b[engine.offset(0x101F9D90)], 0xB8)
    check('factory getter callback target', engine.word(0x101F9D91), 0x101F9D60)
    check('factory wrapper active-group global', engine.word(0x101F9D68), 0x10671CBC)
    check('temporary factory binding output', engine.word(0x101CEDA3), 0x1066CA74)
    check('temporary factory clear output', engine.word(0x101CEDB2), 0x1066CA74)
    check('temporary factory clear value', engine.word(0x101CEDB6), 0)
    for image, site, opcode, target in ((engine, 0x101F9D6D, 0xE8, 0x101F9B90),
                                        (engine, 0x101CED9A, 0xE8, 0x101F9D90),
                                        (ifm, 0x10A64534, 0xE9, 0x10400B00),
                                        (ifm, 0x10989D89, 0xE8, 0x10A64530),
                                        (ifm, 0x10989D94, 0xE8, 0x10A64800),
                                        (ifm, 0x10989D9A, 0xE8, 0x10A35390),
                                        (ifm, 0x10989DA5, 0xE8, 0x106D6C60)):
        offset = image.offset(site)
        check('branch opcode ' + hex(site), image.b[offset], opcode)
        check('branch target ' + hex(site), site + 5 + struct.unpack_from('<i', image.b, offset + 1)[0], target)
    check('registration name-table reference', ifm.word(0x10400B5B), 0x10EA0A70)
    check('registration output-table reference', ifm.word(0x10400B52), 0x10EA0A74)
    stride = ifm.b[ifm.offset(0x10400B92)]
    span = ifm.word(0x10400B95)
    check('record stride', stride, 8)
    check('record span', span, 0x1C0)
    check('fourth Connect helper is a no-op RET', ifm.b[ifm.offset(0x106D6C60)], 0xC3)
    table = []
    for index in range(span // stride):
        entry = 0x10EA0A70 + index * stride
        table.append(dict(index=index, entry=hex(entry),
                          name=ifm.string(ifm.word(entry)), output_slot=hex(ifm.word(entry + 4))))
    by_slot = {row['output_slot']: row['name'] for row in table}
    check('unique output slots', len(by_slot), 56)
    check('Cvar notification global identity', by_slot['0x10f4c744'], 'VEngineCvar007')
    check('UI global identity', by_slot['0x10f4c7e4'], 'VGUI_ivgui008')
    check('material-system global identity A', by_slot['0x10f4c774'], 'VMaterialSystem080')
    check('material-system global identity B', by_slot['0x10f4c778'], 'VMaterialSystem080')
    vtable = 0x1036172C
    type_descriptor = engine.word(engine.word(vtable - 4) + 12)
    check('stack group RTTI class', engine.string(type_descriptor + 8), '.?AVCModAppSystemGroup@@')
    output.parent.mkdir(parents=True, exist_ok=True)
    result = dict(passed=True, scope='Static routing/binding metadata only; original code never executed',
                  sha256=hashes, checks=checks, check_count=len(checks), interface_registry=table,
                  record_count=len(table), distinct_names=len({row['name'] for row in table}),
                  requirement_limit='Queries are not all mandatory; failure conditions require caller inspection.')
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(passed=True, checks=len(checks), records=len(table), distinct_names=result['distinct_names'])))


if __name__ == '__main__':
    main()
