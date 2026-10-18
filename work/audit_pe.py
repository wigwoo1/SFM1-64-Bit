"""Read-only PE inventory. Uses only Python's standard library."""
import argparse, collections, hashlib, json, pathlib, re, struct

def inspect(path, root):
    b = path.read_bytes()
    def u16(o): return struct.unpack_from('<H', b, o)[0]
    def u32(o): return struct.unpack_from('<I', b, o)[0]
    p = u32(60)
    if b[p:p+4] != b'PE\0\0': raise ValueError('Not PE')
    machine, count, optsize = u16(p+4), u16(p+6), u16(p+20)
    opt = p+24
    magic = u16(opt)
    directory = opt + (112 if magic == 0x20b else 96)
    sections = []
    for i in range(count):
        o = opt+optsize+40*i
        sections.append((u32(o+12), max(u32(o+8), u32(o+16)), u32(o+20)))
    def offset(rva):
        for va, size, raw in sections:
            if va <= rva < va+size: return raw+rva-va
        if rva < u32(opt+60): return rva
        raise ValueError('Unmapped RVA '+hex(rva))
    def string(o):
        end = b.find(b'\0', o)
        return b[o:end].decode('ascii', errors='replace')
    imports = []
    rva = u32(directory+8)
    if rva:
        o = offset(rva)
        while any(b[o:o+20]):
            imports.append(string(offset(u32(o+12))))
            o += 20
    exports = []
    rva = u32(directory)
    if rva:
        o = offset(rva)
        names = offset(u32(o+32)) if u32(o+24) else 0
        for i in range(u32(o+24)):
            exports.append(string(offset(u32(names+4*i))))
    strings = [s.decode('ascii') for s in re.findall(rb'[ -~]{6,}', b)]
    return dict(path=str(path.relative_to(root)), bytes=len(b), sha256=hashlib.sha256(b).hexdigest(),
                machine=hex(machine), bits=64 if magic==0x20b else 32,
                large_address_aware=bool(u16(p+22)&32), imports=imports, exports=exports,
                pdb_paths=[s for s in strings if s.lower().endswith('.pdb')],
                interface_candidates=sorted(set(s for s in strings if re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]{3,80}[0-9]{3}',s))),
                sfm_strings=sorted(set(s for s in strings if re.search(r'filmmaker|sfm[_.]|movieobjects|dmeclip',s,re.I)))[:150])

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('root',type=pathlib.Path)
    ap.add_argument('output',type=pathlib.Path)
    args=ap.parse_args()
    records=[]; errors=[]
    for p in sorted(args.root.rglob('*')):
        if p.is_file() and p.suffix.lower() in ('.exe','.dll','.pyd','.asi','.flt'):
            try: records.append(inspect(p,args.root))
            except Exception as e: errors.append(dict(path=str(p),error=str(e)))
    data=dict(root=str(args.root), modules=records, errors=errors)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(data,indent=2),encoding='utf-8')
    print('Modules:',len(records),'Errors:',len(errors))
    print('Architectures:',dict(collections.Counter(x['bits'] for x in records)))
    for x in records:
        if x['path']=='sfm.exe' or any('filmmaker' in s.lower() for s in x['sfm_strings']):
            print(x['path'], 'imports:', ', '.join(x['imports']), 'PDB:',x['pdb_paths'])

if __name__=='__main__': main()
