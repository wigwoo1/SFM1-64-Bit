"""Fresh x64 byte-packer test versus static replay and literal byte predictions."""
import hashlib, json, os, random, struct, subprocess, sys, tempfile, re
from pathlib import Path
from datetime import datetime, timezone
ROOT=Path(__file__).resolve().parent.parent
AUDIT=ROOT/'outputs/native-hdr-selector-20261102'
sys.path.insert(0,str(AUDIT))
import verify_hdr0_pixel_store as reference
import verify_lightmap_page_allocator as pages

def need(ok,msg):
    if not ok: raise RuntimeError(msg)
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def predicted(fmt,slots):
    r,g,b,a=(v&255 for v in slots)
    if fmt==21: return 4,bytes((b,g,r,a))
    if fmt==26: return 2,struct.pack('<H',((a>>4)<<12)|((r>>4)<<8)|((g>>4)<<4)|(b>>4))
    if fmt in (36,113): return 8,struct.pack('<4H',r,g,b,a)
    return None
def main():
    run=Path(tempfile.mkdtemp(prefix='native-hdr0-pixel-',dir=ROOT/'outputs')).resolve()
    receipt=dict(status='FAILED',directory=str(run),started_utc=datetime.now(timezone.utc).isoformat(),commands=[])
    def command(args,label,expected=0):
        r=subprocess.run(args,cwd=ROOT,capture_output=True,timeout=180,check=False)
        (run/(label+'.stdout.log')).write_bytes(r.stdout); (run/(label+'.stderr.log')).write_bytes(r.stderr)
        receipt['commands'].append(dict(command=[str(a) for a in args],label=label,exit_code=r.returncode,expected_exit_code=expected))
        need(r.returncode==expected,label+' unexpected exit; see '+str(run)); return r
    try:
        bs=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text(encoding='utf-8'))
        protected=[ROOT/x for x in bs['editor_source_dependencies']]+[ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe',pages.DLL,pages.SHADER_DLL]
        before={str(x):sha(x) for x in protected}
        command([sys.executable,str(AUDIT/'verify_hdr0_pixel_store.py')],'static-replay-prerequisite')
        image,shader=pages.Image(pages.DLL),pages.Image(pages.SHADER_DLL)
        need(sha(pages.DLL)==pages.EXPECTED_DLL_SHA256 and sha(pages.SHADER_DLL)==pages.EXPECTED_SHADER_DLL_SHA256,'Pinned DLL changed')
        instruction_rows=pages.collect_instructions(pages.DISASM,pages.RANGES+reference.HELPERS)
        pages.authenticate_ranges(image,instruction_rows,pages.RANGES+reference.HELPERS)
        rows={row['address']:row for row in instruction_rows}
        sr=pages.collect_instructions(pages.SHADER_DISASM,pages.SHADER_RANGES)
        pages.authenticate_ranges(shader,sr,pages.SHADER_RANGES)
        formats=pages.verify_pixel_writer_formats(shader,sr)['supported_formats']
        vectors=[tuple(v if c==i else 0 for c in range(4)) for i in range(4) for v in range(256)]
        rng=random.Random(0x53464D); vectors += [tuple(rng.getrandbits(32) for _ in range(4)) for _ in range(256)]
        vectors += [(0,0,0,0),(255,255,255,255),(0x12,0x34,0x56,0x78)]
        need(len(vectors)==1283,'Reference vector set changed')
        records=[]; expected=[]; comparisons=0
        for fmt in (21,26,36,113):
            layout=formats[str(fmt)]
            for slots in vectors:
                records.append(struct.pack('<5I',fmt,*slots))
                size,pix=predicted(fmt,slots)
                packed,trace=reference.replay(rows,image,layout,slots,0xA5)
                expected.append(bytes((1,size))+pix+bytes([0xA5])*(8-size))
                need(packed[8:8+size]==pix and packed[:8]==bytes([0xA5])*8 and packed[8+size:]==bytes([0xA5])*(24-size),'Static replay/literal mismatch')
                need(sum(n for _,n in trace)==size,'Static store width mismatch')
                comparisons+=1
        unsupported=[fmt for fmt in range(256) if fmt not in (21,26,36,113)]
        unsupported += [256,65535,65536,0x7fffffff,0x80000000,0xffffffff]
        need(len(unsupported)==258,'Unsupported format corpus must contain 258 IDs')
        for fmt in unsupported:
            records.append(struct.pack('<5I',fmt,0x12345678,0xabcdef01,0xdeadbeef,0xfedcba98))
            expected.append(bytes((0,0x5A))+bytes([0xA5])*8)
        inp=run/'pixels-u32le.bin'; inp.write_bytes(b''.join(records)); exp=b''.join(expected)
        exe=run/'test_native_lightmap_hdr0_pixel_x64.exe'
        command([os.environ.get('COMSPEC','cmd.exe'),'/d','/c',str(ROOT/'work/build_native_lightmap_hdr0_pixel_test.cmd'),str(run)],'build')
        raw=exe.read_bytes(); pe=struct.unpack_from('<I',raw,0x3c)[0]
        need(raw[:2]==b'MZ' and raw[pe:pe+4]==b'PE\0\0','Invalid PE')
        machine=struct.unpack_from('<H',raw,pe+4)[0]; magic=struct.unpack_from('<H',raw,pe+24)[0]
        need((machine,magic)==(0x8664,0x20b),'Not x64 PE32+')
        warnings=re.findall(r'warning [A-Z]+\d+:[^\r\n]*',(run/'build.log').read_text(errors='replace'))
        need(not warnings,'Compiler warnings: '+repr(warnings))
        outdata=[]
        for label in ('first-run','repeat-run'):
            out=run/(label+'.bin'); command([str(exe),str(inp),str(out)],label)
            rawout=out.read_bytes(); need(len(rawout)==len(records)*10,'Short/extra records')
            need(rawout==exp,'Owned output differs from static and literal expectations'); outdata.append(rawout)
        need(outdata[0]==outdata[1],'Repeated runs differ')
        empty=run/'empty.bin'; empty.write_bytes(b''); eo=run/'empty.out'; command([str(exe),str(empty),str(eo)],'empty'); need(eo.read_bytes()==b'','Empty output is not empty')
        partial=run/'partial.bin'; partial.write_bytes(b'\x15\0\0'); command([str(exe),str(partial),str(run/'partial.out')],'partial',6)
        after={str(x):sha(x) for x in protected}; need(before==after,'Protected file hash changed')
        receipt.update(status='VERIFIED',completed_utc=datetime.now(timezone.utc).isoformat(),executable=str(exe),executable_sha256=sha(exe),architecture={'machine':hex(machine),'optional_header_magic':hex(magic)},compiler_warnings=warnings,supported_vectors=comparisons,unsupported_ids=unsupported,record_size=10,repeated_runs=2,input_sha256=sha(inp),output_sha256=hashlib.sha256(outdata[0]).hexdigest(),protected_files_unchanged=len(after),protected_sha256=after,limits=['HDR0 post-quantization packer only','No original DLL execution, renderer integration, device selection or visual parity'])
    finally: (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('commands','protected_sha256')},indent=2))
if __name__=='__main__': main()
