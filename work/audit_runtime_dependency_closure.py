"""Read-only PE import candidate closure; does not load or execute inspected modules.

Local filename matches are candidates, never assertions about Windows loader search order.
Outputs contain metadata only, not binary bytes, game data, PDB paths or disassembly.
"""
import argparse
import collections
import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import struct


def inspect_pe(path):
    data = path.read_bytes()

    def unpack(fmt, offset):
        return struct.unpack_from('<' + fmt, data, offset)[0]

    if data[:2] != b'MZ':
        raise ValueError('Not an MZ image')
    pe = unpack('I', 60)
    if data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('Not a PE image')
    machine = unpack('H', pe + 4)
    optional = pe + 24
    magic = unpack('H', optional)
    if magic not in (0x10B, 0x20B):
        raise ValueError('Unsupported optional header')
    directory = optional + (112 if magic == 0x20B else 96)
    image_base = unpack('Q' if magic == 0x20B else 'I', optional + (24 if magic == 0x20B else 28))
    count = unpack('I', directory - 4)
    section_start = optional + unpack('H', pe + 20)
    sections = []
    for i in range(unpack('H', pe + 6)):
        entry = section_start + i * 40
        sections.append((unpack('I', entry + 12), unpack('I', entry + 16), unpack('I', entry + 20)))

    def raw(rva):
        if rva < unpack('I', optional + 60) and rva < len(data):
            return rva
        for va, size, offset in sections:
            if va <= rva < va + size and offset + rva - va < len(data):
                return offset + rva - va
        raise ValueError('RVA has no file-backed bytes: ' + hex(rva))

    def cstring(rva):
        offset = raw(rva)
        end = data.find(b'\0', offset)
        if end < 0:
            raise ValueError('Unterminated import name')
        return data[offset:end].decode('ascii')

    def imports(index, stride, name_offset, delayed=False):
        if index >= count:
            return []
        rva, size = struct.unpack_from('<II', data, directory + index * 8)
        if not rva or not size:
            return []
        result = []
        for consumed in range(0, size - stride + 1, stride):
            offset = raw(rva + consumed)
            if not any(data[offset:offset + stride]):
                return result
            name = unpack('I', offset + name_offset)
            if delayed and not (unpack('I', offset) & 1):
                name -= image_base
            result.append(cstring(name))
        raise ValueError('Import descriptors lack terminator within directory size')

    normal = imports(1, 20, 12)
    delayed = imports(13, 32, 4, True)
    candidates = sorted({s.decode('ascii') for s in re.findall(rb'[ -~]{5,}', data)
                         if re.fullmatch(rb'[A-Za-z0-9_./\\-]+\.dll', s, re.I)})
    return dict(machine=hex(machine), architecture={0x14C: 'x86', 0x8664: 'x64'}.get(machine, 'other'),
                optional_header=hex(magic), sha256=hashlib.sha256(data).hexdigest(), bytes=len(data),
                imports=normal, delay_imports=delayed, dll_string_candidates=candidates)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--owned-editor', type=Path)
    args = parser.parse_args()
    root = args.root.resolve(strict=True)
    output = args.output.resolve()
    if output == root or root in output.parents:
        parser.error('Output must be outside the read-only original installation')
    index = collections.defaultdict(list)
    for parent, dirs, files in os.walk(root, followlinks=False):
        dirs[:] = sorted(d for d in dirs if not (Path(parent) / d).is_symlink())
        for name in sorted(files):
            path = Path(parent) / name
            if path.suffix.lower() in {'.dll', '.exe', '.pyd', '.asi', '.flt'} and not path.is_symlink():
                index[name.lower()].append(path)
    seeds = ['sfm.exe', 'bin/launcher.dll', 'bin/engine.dll', 'bin/tools/ifm.dll',
             'bin/filesystem_stdio.dll', 'bin/materialsystem.dll', 'bin/shaderapidx9.dll']
    queue = collections.deque(root / name for name in seeds)
    records, edges, errors = {}, [], []
    while queue:
        path = queue.popleft()
        label = path.relative_to(root).as_posix()
        if label in records:
            continue
        # Mark first so cycles and unreadable files cannot create an infinite queue.
        records[label] = None
        try:
            record = inspect_pe(path)
        except (OSError, ValueError, struct.error, UnicodeError) as error:
            errors.append(dict(path=label, error=str(error)))
            continue
        records[label] = record
        for kind in ('imports', 'delay_imports'):
            for name in record[kind]:
                matches = index.get(name.lower(), [])
                edges.append(dict(importer=label, kind=kind, name=name,
                                  resolution='local-file-candidates' if matches else 'external-or-unresolved',
                                  candidates=[p.relative_to(root).as_posix() for p in matches]))
                queue.extend(matches)
    owned = None
    if args.owned_editor:
        owned = inspect_pe(args.owned_editor.resolve(strict=True))
        owned['path'] = str(args.owned_editor.resolve())
        owned['import_names_also_present_in_original_tree'] = sorted({n for kind in ('imports', 'delay_imports')
                                                                    for n in owned[kind] if n.lower() in index})
    result = dict(schema=1, checked_at=datetime.datetime.now(datetime.timezone.utc).isoformat(),
                  root=str(root), explicit_seeds=seeds, indexed_binary_files=sum(map(len, index.values())),
                  scope='Overapproximation of local ordinary/delay import filename candidates from explicit seeds; not actual loaded-module order.',
                  dynamic_load_limit='DLL strings are unverified candidates; no claim that they are loaded or all dynamic loads are found.',
                  provenance_limit='Files under the installation are not automatically Valve-owned; third-party provenance requires separate review.',
                  modules=records, edges=edges, errors=errors, owned_editor=owned)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(dict(indexed=result['indexed_binary_files'], reached=len(records),
                          architectures=dict(collections.Counter(r['architecture'] for r in records.values() if r)),
                          edges=len(edges), errors=len(errors), owned_architecture=owned and owned['architecture'])))
    return 1 if errors else 0


if __name__ == '__main__':
    raise SystemExit(main())
