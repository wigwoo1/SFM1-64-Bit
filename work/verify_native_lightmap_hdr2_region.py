"""Fresh owned x64 HDR2 region test with independent byte/address evidence.

Input records: u32 payload length, then <5IQ6I> (format,W,H,x,y,stride,
presence mask,destination length,four source pixel counts), source RGBA u32
vectors in plane order, destination bytes. Output: u8 success,u32 length,
destination bytes,u8 sources-unchanged. All fields are little endian.
Harness limits: payload16MiB, destination4MiB, each source65536 pixels.
Empty input exits0; truncated records6; invalid/over-limit records8.
"""
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parent.parent
AUDIT = ROOT / 'outputs/native-hdr-selector-20261102'
sys.path.insert(0, str(AUDIT))
import verify_lightmap_page_allocator as p
import verify_half_conversion_contract as half
from verify_hdr2_single_pack import replay

PALETTE = (0,0x80000000,0x33800000,0xb3800000,0x337fffff,0x3f801fff,
           0x3f802000,0x477fe000,0x7f800000,0xff800000,0x7fc12345,
           0xff800001,0x12345678,0x89abcdef,0x3f000000,0x40000000)


def need(ok, message):
    if not ok: raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def source(n, plane):
    return [tuple(PALETTE[(i*3+plane*5+c*7)%len(PALETTE)] for c in range(4)) for i in range(n)]


def case(name, fmt=113, w=2, h=3, x=1, y=1, stride=None, mask=15, tail=13):
    n = 4 if mask == 15 else 1
    if stride is None: stride = (x+n*w)*8+24
    size = (y+h-1)*stride+(x+n*w)*8+tail
    return dict(name=name, fmt=fmt,w=w,h=h,x=x,y=y,stride=stride,mask=mask,
                sources=[source(w*h,k) if mask & (1<<k) else [] for k in range(4)],
                destination=bytes((i*37+19)%251 for i in range(size)), success=True)


def serialize(c):
    sources = c['sources']
    body = struct.pack('<5IQ6I',c['fmt'],c['w'],c['h'],c['x'],c['y'],c['stride'],
                       c['mask'],len(c['destination']),*(len(v) for v in sources))
    body += b''.join(struct.pack('<4I',*v) for plane in sources for v in plane)
    body += c['destination']
    return struct.pack('<I',len(body))+body


def main():
    run = Path(tempfile.mkdtemp(prefix='native-lightmap-hdr2-region-',dir=ROOT/'outputs')).resolve()
    need(run.is_relative_to((ROOT/'outputs').resolve()),'Output escaped workspace')
    print('Verification directory: '+str(run),flush=True)
    receipt = dict(status='FAILED',started_utc=datetime.now(timezone.utc).isoformat(),directory=str(run),commands=[])

    def command(args,label,expected=0):
        result = subprocess.run(args,cwd=ROOT,capture_output=True,check=False,timeout=240)
        (run/(label+'.stdout.log')).write_bytes(result.stdout)
        (run/(label+'.stderr.log')).write_bytes(result.stderr)
        receipt['commands'].append(dict(command=[str(a) for a in args],label=label,
                                        exit_code=result.returncode,expected_exit_code=expected))
        need(result.returncode==expected,label+' returned '+str(result.returncode))
        return result

    try:
        bs = json.loads((ROOT/'publication/stage/docs/build-status.json').read_text())
        protected = [ROOT/name for name in bs['editor_source_dependencies']]
        protected += [ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe',p.DLL,p.SHADER_DLL]
        protected += [ROOT/('work/native_lightmap_'+s+'_candidate.h') for s in ('half','pixel_layout','hdr2_pixel')]
        before = {str(path):sha(path) for path in protected}
        command([sys.executable,str(AUDIT/'verify_hdr2_region_addressing.py')],'static-region-prerequisite')
        image,shader = p.Image(p.DLL),p.Image(p.SHADER_DLL)
        need(sha(p.DLL)==p.EXPECTED_DLL_SHA256 and sha(p.SHADER_DLL)==p.EXPECTED_SHADER_DLL_SHA256,'Pinned DLL hash changed')
        rows_list = p.collect_instructions(p.DISASM,p.RANGES)
        p.authenticate_ranges(image,rows_list,p.RANGES)
        rows = {r['address']:r for r in rows_list}
        sr = p.collect_instructions(p.SHADER_DISASM,p.SHADER_RANGES)
        p.authenticate_ranges(shader,sr,p.SHADER_RANGES)
        formats = p.verify_pixel_writer_formats(shader,sr)['supported_formats']
        hr_list = p.collect_instructions(p.DISASM,half.RANGE)
        p.authenticate_ranges(image,hr_list,half.RANGE)
        hr = {r['address']:r for r in hr_list}
        half_cache, pixel_cache = {},{}

        def half_value(bits):
            if bits not in half_cache:
                ref = half.replay(bits,hr,image)
                need(ref==half.reduced_contract(bits),'Half formula/reference disagree')
                half_cache[bits]=ref
            return half_cache[bits]

        def pixel(fmt,bits,plane):
            key = (fmt,bits,plane)
            if key not in pixel_cache:
                words = tuple(half_value(v) for v in bits) if fmt==113 else bits
                literal = struct.pack('<4H',*(half.reduced_contract(v) if fmt==113 else v&65535 for v in bits))
                for fill in (0xa5,0x5a):
                    actual = replay(rows,formats[str(fmt)],words,fill,bump_plane=plane)
                    need(actual==literal,'Pinned pixel block disagrees with literal encoding')
                pixel_cache[key]=literal
            return pixel_cache[key]

        cases=[]
        for fmt,bump_mask,geometry,exact in itertools.product((36,113),range(8),
                ((1,1,0,0,0),(3,2,2,1,17),(2,5,1,3,0),(7,3,0,0,31)),(False,True)):
            w,h,x,y,pad=geometry
            mask=1|(bump_mask<<1); n=4 if mask==15 else 1
            cases.append(case('geometry-%d-%d-%s-%s'%(fmt,bump_mask,geometry,exact),fmt,w,h,x,y,
                              (x+n*w)*8+pad,mask,0 if exact else 29))
        for fmt,mask in itertools.product((36,113),(1,15)):
            cases.append(case('wide-stride-%d-%d'%(fmt,mask),fmt,2,2,1,1,70001,mask,0))
            cases.append(case('large-count-%d-%d'%(fmt,mask),fmt,129,129,0,0,None,mask,0))
        # Pair demonstrates ignored bump alpha, including NaN/infinity alpha.
        original=case('bump-alpha-original')
        altered=case('bump-alpha-ignored')
        for k in range(1,4): altered['sources'][k]=[(*v[:3],v[3]^0xffffffff) for v in altered['sources'][k]]
        cases.extend([original,altered])

        invalid=[]
        def reject(name,**changes):
            c=case(name);c.update(changes);c['success']=False;invalid.append(c);return c
        for fmt in (0,21,26,35,37,112,114,255,256,65535,0xffffffff): reject('unsupported-'+str(fmt),fmt=fmt)
        reject('main-absent',mask=14)
        reject('zero-width',w=0);reject('zero-height',h=0)
        reject('zero-stride',stride=0);reject('row-too-short',stride=71)
        c=reject('destination-short');c['destination']=c['destination'][:(1+3-1)*96+72-1]
        for k in range(4):
            c=reject('short-plane-'+str(k));c['sources'][k]=c['sources'][k][:-1]
            c=reject('extra-plane-'+str(k));c['sources'][k].append((1,2,3,4))
        # Full size_t boundary operands are carried through the protocol without
        # allocating simulated source/destination extents.
        reject('row-multiply-overflow',stride=(1<<64)-1)
        reject('last-byte-add-overflow',w=1,h=1,x=0,y=1,stride=(1<<64)-8)
        reject('maximum-u32-dimensions',w=0xffffffff,h=0xffffffff,x=0xffffffff,y=0xffffffff,stride=(1<<64)-1)
        reject('huge-offset-capacity',x=0xffffffff,y=0xffffffff)
        # Incomplete bump sets ignore their provided vectors, including wrong
        # unused lengths, matching original route selection.
        c=case('partial-unused-wrong-size',mask=3);c['sources'][1]=[];cases.append(c)
        cases += invalid
        expected_chunks=[];expected_buffers=[];pixels_checked=0
        for c in cases:
            result=bytearray(c['destination'])
            if c['success']:
                n=4 if c['mask']==15 else 1
                for y in range(c['h']):
                    for x in range(c['w']):
                        i=y*c['w']+x
                        for k in range(n):
                            bits=(*c['sources'][k][i][:3],c['sources'][0][i][3])
                            data=pixel(c['fmt'],bits,k if n==4 else None)
                            dest=(c['y']+y)*c['stride']+(c['x']+x+k*c['w'])*8
                            need(dest+8<=len(result),'Invalid positive fixture extent')
                            result[dest:dest+8]=data;pixels_checked+=1
            expected_buffers.append(bytes(result))
            expected_chunks.append(bytes([int(c['success'])])+struct.pack('<I',len(result))+result+b'\x01')
        a=next(i for i,c in enumerate(cases) if c['name']=='bump-alpha-original')
        b=next(i for i,c in enumerate(cases) if c['name']=='bump-alpha-ignored')
        need(expected_buffers[a]==expected_buffers[b],'Ignored bump alpha fixture disagrees')
        inputs=run/'region-records.bin';inputs.write_bytes(b''.join(serialize(c) for c in cases))
        expected=b''.join(expected_chunks)
        executable=run/'test_native_lightmap_hdr2_region_x64.exe'
        command([os.environ.get('COMSPEC','cmd.exe'),'/d','/c',str(ROOT/'work/build_native_lightmap_hdr2_region_test.cmd'),str(run)],'build')
        raw=executable.read_bytes();pe=struct.unpack_from('<I',raw,0x3c)[0]
        machine=struct.unpack_from('<H',raw,pe+4)[0];magic=struct.unpack_from('<H',raw,pe+24)[0]
        need(raw[:2]==b'MZ' and raw[pe:pe+4]==b'PE\0\0' and (machine,magic)==(0x8664,0x20b),'Not AMD64 PE32+')
        warnings=re.findall(r'warning [A-Z]+\d+:[^\r\n]*',(run/'build.log').read_text(errors='replace'))
        need(not warnings,'Compiler warnings: '+repr(warnings))
        actual_runs=[]
        for label in ('owned-first','owned-repeat'):
            output=run/(label+'.bin');command([str(executable),str(inputs),str(output)],label)
            actual=output.read_bytes();need(len(actual)==len(expected),'Output length mismatch')
            if actual!=expected:
                cursor=0
                for c,chunk in zip(cases,expected_chunks):
                    need(actual[cursor:cursor+len(chunk)]==chunk,'Wrong whole-buffer/source-canary result: '+c['name'])
                    cursor+=len(chunk)
            actual_runs.append(actual)
        need(actual_runs[0]==actual_runs[1],'Repeated run mismatch')
        for label,data,exit_code in (('empty',b'',0),('partial-length',b'\x34',6),
                ('partial-payload',struct.pack('<I',52)+b'\x00'*7,6),
                ('oversized-record',struct.pack('<I',16*1024*1024+1),8),
                ('invalid-record',struct.pack('<I',52)+b'\x00'*36+struct.pack('<4I',1,0,0,0),8)):
            path=run/(label+'.bin');path.write_bytes(data);out=run/(label+'.out')
            command([str(executable),str(path),str(out)],label,exit_code)
            need(out.read_bytes()==b'','Malformed/empty input unexpectedly emitted records')
        after={str(path):sha(path) for path in protected}
        need(before==after,'Protected editor/reference/helper changed')
        cl=('cl /nologo /std:c++17 /EHsc /W4 /O2 work\\test_native_lightmap_hdr2_region.cpp '
            '/Fo:"'+str(run/'test_native_lightmap_hdr2_region.obj')+'" /Fe:"'+str(executable)+'" '
            '/link /MACHINE:X64 /MANIFEST:EMBED /MANIFESTUAC:"level=\'asInvoker\' uiAccess=\'false\'"')
        receipt.update(status='VERIFIED ISOLATED OWNED HDR2 REGION ENCODER',completed_utc=datetime.now(timezone.utc).isoformat(),
                       executable=str(executable),executable_sha256=sha(executable),
                       architecture=dict(machine=hex(machine),optional_header_magic=hex(magic)),
                       exact_build_command=['call "C:\\Program Files (x86)\\Microsoft Visual Studio\\18\\BuildTools\\VC\\Auxiliary\\Build\\vcvars64.bat"',cl],
                       compiler_warnings=warnings,records=len(cases),accepted=sum(c['success'] for c in cases),
                       rejected=len(invalid),whole_buffer_pixels_checked=pixels_checked,
                       independent_cached_pixel_block_cases=len(pixel_cache)*2,unique_half_inputs=len(half_cache),
                       repeated_runs=2,input_sha256=sha(inputs),output_sha256=hashlib.sha256(actual_runs[0]).hexdigest(),
                       case_names=[c['name'] for c in cases],protocol=__doc__,
                       protected_files_unchanged=len(after),protected_sha256=after,
                       limits=['Isolated CPU encoder only; no editor or texture adapter integration/rebuild',
                               'No original DLL executed; references are static instruction replays',
                               'No live GPU/visual parity or allocation-failure injection test'])
    finally:
        (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k not in ('commands','protected_sha256','case_names','protocol')},indent=2))


if __name__=='__main__':main()
