"""Fresh x64 real D3D9 HDR2 texture byte/readback and fault verification."""
import hashlib,json,os,re,struct,subprocess,sys,tempfile
from pathlib import Path
from datetime import datetime,timezone
ROOT=Path(__file__).resolve().parent.parent
AUDIT=ROOT/'outputs/native-hdr-selector-20261102'
sys.path.insert(0,str(AUDIT))
import verify_lightmap_page_allocator as p
import verify_half_conversion_contract as half
from verify_hdr2_single_pack import replay
PALETTE=(0,0x80000000,0x33800000,0xb3800000,0x337fffff,0x3f801fff,0x3f802000,0x477fe000,
         0x7f800000,0xff800000,0x7fc12345,0xff800001,0x12345678,0x89abcdef,0x3f000000,0x40000000)
def need(ok,msg):
    if not ok:raise RuntimeError(msg)
def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def main():
    run=Path(tempfile.mkdtemp(prefix='hdr2-texture-',dir=ROOT/'outputs')).resolve()
    print('Verification directory: '+str(run),flush=True)
    result=dict(status='FAILED',started_utc=datetime.now(timezone.utc).isoformat(),directory=str(run),commands=[])
    def command(args,label,timeout=240):
        r=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,check=False,timeout=timeout,creationflags=subprocess.CREATE_NO_WINDOW)
        (run/(label+'.stdout.log')).write_bytes(r.stdout);(run/(label+'.stderr.log')).write_bytes(r.stderr)
        result['commands'].append(dict(command=list(map(str,args)),label=label,exit_code=r.returncode))
        need(r.returncode==0,label+' failed: '+r.stdout.decode(errors='replace')[-1600:]+r.stderr.decode(errors='replace')[-1600:]);return r
    try:
        bs=json.loads((ROOT/'publication/stage/docs/build-status.json').read_text())
        paths=[ROOT/n for n in bs['editor_source_dependencies']]+[ROOT/'outputs/sfm64-editor-x64/sfm64_session_ui.exe',p.DLL,p.SHADER_DLL]
        paths += [ROOT/('work/'+n) for n in ('native_lightmap_hdr2_region_candidate.h','native_lightmap_hdr2_pixel_candidate.h','native_lightmap_half_candidate.h','native_lightmap_pixel_layout_candidate.h','d3d9_prepared_scene_candidate.h','probe_d3d9_prepared_scene_x64.cpp','source_lightmap_float_storage_candidate.h','source_lightmap_integer_storage_candidate.h')]
        before={str(path):sha(path) for path in paths}
        header=ROOT/'work/d3d9_packed_lightmap_texture_candidate.h';header_hash=sha(header)
        checkpoint=json.loads((ROOT/'outputs/hdr2-texture-adapter-20261104/before.json').read_text())
        text=header.read_text();old=text[text.index('    static std::unique_ptr<D3D9PackedLightmapTextureCandidate> prepare('):text.index('    // Borrowed texture remains')]
        need(hashlib.sha256(old.encode()).hexdigest()==checkpoint['old_prepare_sha256'],'Old numerical prepare changed')
        command([sys.executable,ROOT/'work/verify_native_lightmap_hdr2_region.py'],'cpu-region-regression')
        image,shader=p.Image(p.DLL),p.Image(p.SHADER_DLL)
        need(sha(p.DLL)==p.EXPECTED_DLL_SHA256 and sha(p.SHADER_DLL)==p.EXPECTED_SHADER_DLL_SHA256,'Pinned references changed')
        found=p.collect_instructions(p.DISASM,p.RANGES);p.authenticate_ranges(image,found,p.RANGES);rows={r['address']:r for r in found}
        sr=p.collect_instructions(p.SHADER_DISASM,p.SHADER_RANGES);p.authenticate_ranges(shader,sr,p.SHADER_RANGES)
        formats=p.verify_pixel_writer_formats(shader,sr)['supported_formats']
        hr={r['address']:r for r in p.collect_instructions(p.DISASM,half.RANGE)}
        cache={}
        def encoded(fmt,bits,plane):
            key=(fmt,bits,plane)
            if key not in cache:
                if fmt==113:
                    words=tuple(half.replay(v,hr,image) for v in bits)
                    need(words==tuple(half.reduced_contract(v) for v in bits),'Half references differ')
                else:words=bits
                data=struct.pack('<4H',*(half.reduced_contract(v) if fmt==113 else v&65535 for v in bits))
                need(replay(rows,formats[str(fmt)],words,0xa5,bump_plane=plane)==data,'Static pixel pack differs')
                cache[key]=data
            return cache[key]
        def expected(fmt,w,h,mask):
            n=4 if mask==15 else 1;data=bytearray(w*h*n*8)
            for y in range(h):
                for x in range(w):
                    i=y*w+x
                    for k in range(n):
                        bits=tuple(PALETTE[(i*3+k*5+c*7)%16] for c in range(3))+(PALETTE[(i*3+21)%16],)
                        at=(y*n*w+k*w+x)*8;data[at:at+8]=encoded(fmt,bits,k if n==4 else None)
            return bytes(data)
        exe=run/'probe_d3d9_hdr2_texture_x64.exe'
        command([os.environ.get('COMSPEC','cmd.exe'),'/d','/c',ROOT/'work/build_d3d9_hdr2_texture_test.cmd',run],'build')
        raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0]
        need(raw[:2]==b'MZ' and raw[pe:pe+4]==b'PE\0\0' and struct.unpack_from('<H',raw,pe+4)[0]==0x8664 and struct.unpack_from('<H',raw,pe+24)[0]==0x20b,'Not AMD64 PE32+')
        warnings=re.findall(r'[^\r\n]*warning [A-Z]+\d+:[^\r\n]*',(run/'build.log').read_text(errors='replace'))
        need(not any('d3d9_packed_lightmap_texture_candidate.h(' in w or 'probe_d3d9_hdr2_texture_x64.cpp(' in w for w in warnings),'New adapter/probe compiler warnings: '+repr(warnings))
        counts=[];outputs=[]
        for repeat in range(2):
            out=run/('gpu-'+str(repeat));command([exe,out],'gpu-'+str(repeat))
            metrics=dict(line.split('=',1) for line in (out/'metrics.txt').read_text().splitlines() if '=' in line and not line.startswith('layout_skip='))
            data=(out/'textures.bin').read_bytes();at=0;records=0;pixels=0;actual_formats=set();layouts=set()
            while at<len(data):
                serial,fmt,w,h,mask,tw,th,length=struct.unpack_from('<8I',data,at);at+=32
                n=4 if mask==15 else 1;need((tw,th,length)==(n*w,h,n*w*h*8),'Real capture descriptor/length')
                need(data[at:at+length]==expected(fmt,w,h,mask),'Real texture word mismatch fmt='+str(fmt)+' mask='+str(mask));at+=length
                actual_formats.add(fmt);layouts.add((fmt,mask));records+=1;pixels+=n*w*h
            need(at==len(data) and records==int(metrics['real_records']),'Capture completeness')
            old=(out/'old-prepare.bin').read_bytes();at=0;old_count=0
            while at<len(old):
                _,fmt,w,h,mask,tw,th,length=struct.unpack_from('<8I',old,at);at+=32
                words=(0x3400,0x3800,0x3c00,0x3c00) if fmt==113 else (1024,2048,4096,65535)
                need((w,h,mask,tw,th,length)==(3,2,1,3,2,48),'Old prepare metadata')
                need(old[at:at+length]==struct.pack('<4H',*words)*6,'Old numerical prepare regression');at+=length;old_count+=1
            need(old_count==len(actual_formats),'Every supported format old prepare tested')
            guarded=(out/'guards.bin').read_bytes();at=0;guard_count=0
            while at<len(guarded):
                fmt,w,h,mask,pitch,length=struct.unpack_from('<6I',guarded,at);at+=24
                raw_expected=expected(fmt,w,h,mask);row=w*4*8;want=bytearray([0xa5]*length)
                for y in range(h):want[16+y*pitch:16+y*pitch+row]=raw_expected[y*row:(y+1)*row]
                need(guarded[at:at+length]==want,'Guarded padding/large-pitch mismatch');at+=length;guard_count+=1
            need(at==len(guarded) and guard_count==int(metrics['guarded_lock_cases']),'Guard completeness')
            need(int(metrics['resources_restored'])==1 and int(metrics['fault_seam_cases'])==len(actual_formats)*14,'Fault/resource gate')
            need(layouts=={(fmt,mask) for fmt in actual_formats for mask in range(1,16,2)},'All bump presence sets exercised')
            counts.append(dict(records=records,pixels=pixels,formats=sorted(actual_formats),old_prepare_cases=old_count,metrics=metrics))
            outputs.append({name:sha(out/name) for name in ('textures.bin','old-prepare.bin','guards.bin')})
        need(outputs[0]==outputs[1],'Repeated GPU/guard/old bytes differ')
        after={str(path):sha(path) for path in paths};need(before==after and sha(header)==header_hash,'Protected source/editor/reference changed')
        full=counts[0]['formats']==[36,113]
        result.update(status='VERIFIED' if full else 'PARTIALLY VERIFIED',completed_utc=datetime.now(timezone.utc).isoformat(),
                      executable=str(exe),executable_sha256=sha(exe),architecture='AMD64 PE32+',compiler_warnings=warnings,
                      build_script_sha256=sha(ROOT/'work/build_d3d9_hdr2_texture_test.cmd'),
                      build_script=(ROOT/'work/build_d3d9_hdr2_texture_test.cmd').read_text(),gpu_runs=counts,
                      repeated_binary_sha256=outputs,static_pixel_cases=len(cache),protected_files_unchanged=len(after),protected_sha256=after,
                      header_transition=dict(before=checkpoint['header_sha256'],after=header_hash,old_prepare_body_unchanged=True),
                      limits=['Real texture bytes verified; no shader draws or visual/editor parity',
                              'Fault seam HRESULTs use real COM resources but are injected results, not actual driver failures',
                              'Guarded lock rows are test buffers, separate from real texture readbacks',
                              'No automatic fallback, live HDR selection, original execution or editor rebuild'])
    finally:(run/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('commands','protected_sha256','build_script')},indent=2))
if __name__=='__main__':main()
