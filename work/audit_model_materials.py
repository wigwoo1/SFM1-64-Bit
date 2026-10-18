"""MDL49 material metadata and loose VMT resolution candidate."""
import argparse
import json
import struct
from pathlib import Path, PurePosixPath


def decode_materials(mdl):
    def span(offset, count, stride):
        if min(offset, count) < 0 or offset > len(mdl) or count * stride > len(mdl) - offset:
            raise ValueError('MDL material span')
    def word(offset):
        span(offset, 1, 4)
        return struct.unpack_from('<i', mdl, offset)[0]
    def string(offset):
        span(offset, 1, 1)
        end = mdl.find(b'\0', offset)
        if end < 0:
            raise ValueError('unterminated material string')
        return mdl[offset:end].decode('utf-8')
    span(0, 1, 232)
    if mdl[:4] != b'IDST' or word(4) != 49:
        raise ValueError('MDL49 materials required')
    count, start = word(204), word(208)
    span(start, count, 64)
    names = [string(start + n * 64 + word(start + n * 64)) for n in range(count)]
    paths, path_start = word(212), word(216)
    span(path_start, paths, 4)
    directories = [string(word(path_start + n * 4)) for n in range(paths)]
    slots, families, skin_start = word(220), word(224), word(228)
    if min(slots, families) < 0:
        raise ValueError('negative skin dimensions')
    span(skin_start, slots * families, 2)
    skins = []
    for family in range(families):
        indices = list(struct.unpack_from('<' + 'h' * slots, mdl, skin_start + family * slots * 2))
        if any(index < 0 or index >= len(names) for index in indices):
            raise ValueError('skin texture index')
        skins.append(indices)
    return dict(names=names, directories=directories, slots=slots, skins=skins)


def resolve_loose_materials(metadata, content_roots):
    results = []
    for name in metadata['names']:
        candidates = []
        for directory in metadata['directories'] or ['']:
            relative = PurePosixPath((directory.rstrip('/\\') + '/' + name).replace('\\', '/').lstrip('/'))
            if '..' in relative.parts or ':' in str(relative):
                raise ValueError('material path escapes content root')
            for content_root in content_roots:
                path = Path(content_root) / 'materials' / (str(relative) + '.vmt')
                if path.is_file():
                    candidates.append(str(path.resolve()))
        results.append(dict(name=name, candidates=list(dict.fromkeys(candidates))))
    return results


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('input')
    parser.add_argument('output')
    parser.add_argument('content_roots', nargs='+')
    args = parser.parse_args()
    result = decode_materials(Path(args.input).read_bytes())
    result['loose_materials'] = resolve_loose_materials(result, args.content_roots)
    Path(args.output).write_text(json.dumps(result, indent=2), encoding='utf-8')
    print(f"Materials: names={len(result['names'])} slots={result['slots']} families={len(result['skins'])} resolved={sum(bool(m['candidates']) for m in result['loose_materials'])}")
