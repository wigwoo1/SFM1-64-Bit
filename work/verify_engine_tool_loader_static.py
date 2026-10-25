"""Verify recorded static tool-loader landmarks without loading original code.

Addresses and string names are evidence for the pinned x86 images only, never an x64 ABI.
No proprietary instruction byte arrays are embedded in this verifier.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from pe_image import Image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    root = args.root.resolve(strict=True)
    output = args.output.resolve()
    if output == root or root in output.parents:
        parser.error('Output must be outside the read-only installation')
    engine = Image(root / 'bin/engine.dll')
    editor = Image(root / 'bin/tools/ifm.dll')
    hashes = {
        'engine.dll': hashlib.sha256(engine.b).hexdigest(),
        'ifm.dll': hashlib.sha256(editor.b).hexdigest(),
    }
    expected = {
        'engine.dll': 'c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f',
        'ifm.dll': '8576a42dbfc16790cab575a69f89373a8f00cfbfcdd6007a4fc99c0af1425fc4',
    }
    if hashes != expected:
        raise ValueError('Installed binary hash changed; do not apply the recorded addresses')
    checks = []

    def check(label, actual, expected_value):
        if actual != expected_value:
            raise ValueError(label + ': unexpected value ' + repr(actual))
        checks.append(label)

    for site, target in ((0x101E82E4, 0x101E7C50), (0x101E8309, 0x101E7C50),
                         (0x101E8BA7, 0x101E8250), (0x101E7C5F, 0x10251FC0),
                         (0x101E7C87, 0x102521A0), (0x101E7C97, 0x10252180),
                         (0x101E7CC9, 0x10252180), (0x101E7D01, 0x10252180),
                         (0x101E7D2F, 0x10252180)):
        offset = engine.offset(site)
        check('direct-call opcode at ' + hex(site), engine.b[offset], 0xE8)
        check('direct-call target at ' + hex(site),
              site + 5 + struct.unpack_from('<i', engine.b, offset + 1)[0], target)
    for image, site, value in ((engine, 0x101E82DD, 'tools/ifm.dll'),
                               (engine, 0x101E8302, 'tools/pet.dll'),
                               (engine, 0x101E7CB8, 'VTOOLDICTIONARY003'),
                               (engine, 0x102521AC, 'CreateInterface'),
                               (engine, 0x101E8AFD, '-tools'),
                               (editor, 0x10989DB1, 'VEngineVGui001'),
                               (editor, 0x10989DBF, 'VENGINETOOL003'),
                               (editor, 0x10989DCD, 'VDebugOverlay005'),
                               (editor, 0x10989DDB, 'VModelInfoClient004')):
        check('push opcode at ' + hex(site), image.b[image.offset(site)], 0x68)
        check('pushed string at ' + hex(site), image.string(image.word(site + 1)), value)
    slots = (0x10989D70, 0x10989E70, 0x10989C50, 0x10989CC0, 0x10989D40,
             0x1022B000, 0x105D3960, 0x102D3880, 0x10989C80, 0x10989C90, 0x10989CA0)
    for slot, target in enumerate(slots):
        check('IFM dictionary slot ' + str(slot), editor.word(0x10DDB400 + 4 * slot), target)
    output.parent.mkdir(parents=True, exist_ok=True)
    result = dict(passed=True, kind='static landmarks only; no original code executed',
                  sha256=hashes, checks=checks, check_count=len(checks))
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(passed=True, checks=len(checks))))


if __name__ == '__main__':
    main()
