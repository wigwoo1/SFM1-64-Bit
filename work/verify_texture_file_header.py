"""Actual original texture-file open/read/header parser, before image/GPU work."""
from datetime import datetime,timezone
from pathlib import Path
import hashlib,itertools,json,re,struct,subprocess,tempfile
from verify_session_preview_aspect import ROOT,VS
from verify_ifm_draw_packet import check_ranges

OUT=ROOT/'outputs/texture-file-header-20261101'
LIB=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\materialsystem.dll')
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(ok,why):
    if not ok:raise RuntimeError(why)
def text(s):
    b=str(s).encode('ascii');return struct.pack('<I',len(b))+b
def vtf(minor=2,w=2,h=2,frames=1,fmt=0,flags=0):
    header=64 if minor==1 else 80 if minor==2 else 88
    size=w*h*4 if fmt in [0,12] else ((w+3)//4)*((h+3)//4)*8
    b=bytearray(header+size*frames)
    for o,v in [(0,0x00465456),(4,7),(8,minor),(12,header),(20,flags),(52,fmt),(57,0xffffffff)]:struct.pack_into('<I',b,o,v)
    for o,v in [(16,w),(18,h),(24,frames)]:struct.pack_into('<H',b,o,v)
    if minor>1:struct.pack_into('<H',b,63,1)
    if minor>2:struct.pack_into('<I',b,68,1);struct.pack_into('<2I',b,80,0x30,header)
    struct.pack_into('<f',b,48,1);b[56]=1
    return b

def main():
    OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));(OUT/'active-run.txt').write_text(str(run));print('Verification directory: '+str(run),flush=True)
    idx=json.loads((ROOT/'outputs/texture-manager-miss-20261101/latest-verification.json').read_text());need(sha(idx['receipt'])==idx['receipt_sha256'],'accepted cache receipt');prior=json.loads(Path(idx['receipt']).read_text());sources=dict(prior['source_sha256']);protected=dict(prior['protected_sha256']);protected[idx['receipt']]=idx['receipt_sha256'];need(all(sha(p)==h for p,h in sources.items()),'prior source identities');need(all(sha(p)==h for p,h in protected.items()),'prior protected identities')
    visited=set()
    def visit(p):
        p=p.resolve()
        if p in visited:return
        visited.add(p);sources[str(p)]=sha(p)
        for inc in re.findall(r'^\s*#include\s*"([^"\r\n]+)"',p.read_text(),re.M):
            if (p.parent/inc).exists():visit(p.parent/inc)
    for n in ['probe_texture_file_header.cpp','vtf_effective_flags_candidate.h','probe_vtf_effective_flags.cpp',Path(__file__).name]:visit(ROOT/'work'/n)
    image,ranges=check_ranges(LIB,'materialsystem-disassembly.txt',[
        ('reader_prefix',0x1002fdb0,0x1002ff3b),('buffer_constructor',0x1006f6d0,0x1006f749),
        ('scratch_buffer',0x1002e5d0,0x1002e691),('VTF_factory',0x1007c7c0,0x1007c7db),
        ('VTF_constructor',0x1007bef0,0x1007c004),('VTF_header_parse_prefix',0x1007c810,0x1007c93e),
        ('VTF_width',0x1002e490,0x1002e494),('VTF_height',0x10073fe0,0x10073fe4),
        ('VTF_depth',0x1002ce40,0x1002ce44),('VTF_format',0x10078cf0,0x10078cf4),
        ('VTF_frames',0x10078d00,0x10078d04),('VTF_flags',0x10078d10,0x10078d14),
        ('current_header_size',0x10078890,0x100788e6),('legacy_header_conversion',0x1007b300,0x1007b41a)])
    need(image.word(0x100dfa34+32)==0x1007c810,'VTF actual Unserialize slot8');need(image.string(0x10095d6c)=='rb' and image.string(0x1009ae60)=='.hdr.vtf' and image.string(0x1009ae58)=='.vtf','native open/fallback literals')
    need([image.word(0x1007b41c+4*i) for i in range(4)]==[0x1007b3ef,0x1007b3ef,0x1007b3f8,0x1007b3ff],'native version-conversion jump targets')
    need(any(i['bytes']=='816714fffb872e' for i in next(x for x in ranges if x['name']=='legacy_header_conversion')['instructions']),'actual legacy flag mask instruction')
    fixtures=run/'fixtures';fixtures.mkdir();rows=[];coverage={}
    def add(data,accepted,meta=(0,0,0,0,0,0),mode=1,name='materials/fixture/test.vtf',label='valid'):
        serial=len(rows);source=fixtures/(str(serial)+'.vtf');source.write_bytes(data);rows.append({'serial':serial,'source':str(source),'source_sha256':sha(source),'logical':name,'texture':'fixture/test','mode':mode,'accepted':accepted,'meta':meta,'size':len(data),'header':struct.unpack_from('<I',data,12)[0] if len(data)>=16 else 0});coverage[label]=coverage.get(label,0)+1
    for minor,(w,h),frames,fmt,flags in itertools.product(range(1,6),[(2,2),(4,8),(8,4)],[1,2],[0,12,13],[0,0x1000,0x2000,0x3000,0x80000,0x4000]):
        accepted=not(flags&0x4000 and w!=h);effective=flags&0x2e87fbff if minor<=3 else flags;add(vtf(minor,w,h,frames,fmt,flags),accepted,(w,h,1,fmt,frames,effective) if accepted else (0,)*6,label='VTF_versions_formats_dimensions_flags')
    for minor,bit,complement in itertools.product(range(1,6),range(32),[False,True]):
        flags=((1<<bit)^0xffffffff) if complement else 1<<bit;effective=flags&0x2e87fbff if minor<=3 else flags
        add(vtf(minor,flags=flags),True,(2,2,1,0,1,effective),label='legacy_flag_bit_and_complement_matrix')
    for minor in range(1,6):
        data=vtf(minor);header=struct.unpack_from('<I',data,12)[0];add(data[:header],True,(2,2,1,0,1,0),label='header_only_no_pixel_data')
    for name in ['materials/fixture/test.vtf','materials/fixture/test.hdr.vtf']:
        add(vtf(),False,mode=0,name=name,label='all_opens_missing')
    add(vtf(),True,(2,2,1,0,1,0),mode=2,name='materials/fixture/test.hdr.vtf',label='HDR_name_fallback')
    for mode in ['signature','major','minor','width','depth','short']:
        b=vtf()
        if mode=='signature':b[:4]=b'BAD!'
        if mode=='major':struct.pack_into('<I',b,4,8)
        if mode=='minor':struct.pack_into('<I',b,8,99)
        if mode=='width':struct.pack_into('<H',b,16,0)
        if mode=='depth':struct.pack_into('<H',b,63,0)
        if mode=='short':b=b[:8]
        add(b,False,label='invalid_'+mode)
    (run/'cases.json').write_text(json.dumps(rows,indent=2));query=run/'queries.bin';query.write_bytes(struct.pack('<I',len(rows))+b''.join(struct.pack('<I',r['mode'])+text(r['logical'])+text(r['texture'])+text(r['source']) for r in rows))
    batch=run/'build.cmd';batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS.parent/"vcvars32.bat"}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_texture_file_header.cpp /Fo:"{run/"native.obj"}" /Fe:"{run/"native.exe"}" >"{run/"build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0','']))
    def execute(args,label,ok=True):
        p=subprocess.run(list(map(str,args)),cwd=ROOT,capture_output=True,text=True,timeout=120);(run/(label+'.log')).write_text(p.stdout+p.stderr)
        if ok:need(p.returncode==0,label+' failed: '+(p.stdout+p.stderr)[-1600:])
        return p
    execute(['cmd.exe','/d','/c',batch],'build-driver');warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'build.log').read_text());need(not warnings,'compiler warnings');exe=run/'native.exe';raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];need(struct.unpack_from('<H',raw,pe+4)[0]==0x14c and struct.unpack_from('<H',raw,pe+24)[0]==0x10b,'explicit x86 reference')
    traces=[];metadata=[]
    for repeat in range(2):
        target=run/('native-'+str(repeat)+'.bin');execute([exe,LIB,query,target],'native-'+str(repeat));data=target.read_bytes();traces.append(data);offset=0;accepted=0;warnings_seen=0
        for r in rows:
            record=struct.unpack_from('<11I',data,offset);offset+=44;strings=[]
            for _ in range(2):n=struct.unpack_from('<I',data,offset)[0];offset+=4;strings.append(data[offset:offset+n].decode('ascii'));offset+=n
            filename,trace=strings;fallback='.hdr.vtf' in r['logical'] and r['mode']!=1;expected_name=r['logical'].split('.hdr.vtf')[0]+'.vtf' if fallback else r['logical'];opens=2 if fallback else 1;reads=int(r['mode']!=0);expected=[r['serial'],int(r['accepted']),opens,reads,int(bool(reads and not r['accepted'])),*r['meta']]
            if list(record)!=expected or filename!=expected_name:(run/'mismatch.json').write_text(json.dumps({'row':r,'actual_record':record,'expected_record':expected,'actual_filename':filename,'expected_filename':expected_name,'trace':trace},indent=2));raise RuntimeError('header outcome/metadata mismatch case '+str(r['serial']))
            steps=[x for x in trace.splitlines() if x!='W'];expected_trace=[]
            for attempt in range(opens):expected_trace += ['P GAME','O '+(r['logical'] if attempt==0 else expected_name)+' rb 0 GAME']
            if reads:expected_trace += ['A 0','R 2097152 336 '+str(min(r['size'],336)),'S '+str(r['header'])]
            if reads and not r['accepted']:expected_trace+=['X']
            expected_trace+=['HEADER ACCEPTED' if r['accepted'] else 'ERROR DISPATCH'];need(steps==expected_trace,'ordered filesystem trace case '+str(r['serial']));warnings_seen+=trace.splitlines().count('W');accepted+=int(r['accepted'])
            if repeat==0:metadata.append({'serial':r['serial'],'accepted':bool(record[1]),'opens':record[2],'reads':record[3],'closed_before_stop':record[4],'VTF_metadata':list(record[5:]),'logical_after_fallback':filename,'warnings':trace.splitlines().count('W')})
        need(offset==len(data),'complete trace framing')
    need(traces[0]==traces[1],'repeated complete trace identity');(run/'metadata.json').write_text(json.dumps(metadata,indent=2));before=sha(query);p=execute([exe,LIB,query,query],'existing-output',False);need(p.returncode!=0 and sha(query)==before,'existing output preservation')
    owned_batch=run/'owned-build.cmd';owned_batch.write_text('\n'.join(['@echo off',f'cd /d "{ROOT}"',f'call "{VS}" >nul','if errorlevel 1 exit /b 1',f'cl /nologo /std:c++17 /fp:strict /EHsc /W4 /O2 work\\probe_vtf_effective_flags.cpp /Fo:"{run/"owned.obj"}" /Fe:"{run/"owned.exe"}" >"{run/"owned-build.log"}" 2>&1','if errorlevel 1 exit /b 1','exit /b 0','']))
    execute(['cmd.exe','/d','/c',owned_batch],'owned-build-driver');owned_warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(run/'owned-build.log').read_text());need(not owned_warnings,'owned warnings');owned=run/'owned.exe';b=owned.read_bytes();pe=struct.unpack_from('<I',b,60)[0];need(struct.unpack_from('<H',b,pe+4)[0]==0x8664 and struct.unpack_from('<H',b,pe+24)[0]==0x20b,'owned architecture')
    flag_query=bytearray();flag_expected=bytearray();flag_cases=0
    for row,observed in zip(rows,metadata):
        if not observed['accepted']:continue
        b=Path(row['source']).read_bytes();major,minor=struct.unpack_from('<2I',b,4);raw_flags=struct.unpack_from('<I',b,20)[0];flag_query.extend(struct.pack('<3I',major,minor,raw_flags));flag_expected.extend(struct.pack('<2I',1,observed['VTF_metadata'][5]));flag_cases+=1
    for major,minor in [(8,2),(7,0),(7,6),(7,99)]:flag_query.extend(struct.pack('<3I',major,minor,0xffffffff));flag_expected.extend(struct.pack('<2I',0,0xaabbccdd));flag_cases+=1
    fq=run/'flags-query.bin';fq.write_bytes(flag_query)
    for repeat in range(2):
        dest=run/('flags-'+str(repeat)+'.bin');execute([owned,fq,dest],'owned-flags-'+str(repeat));need(dest.read_bytes()==flag_expected,'owned flags vs actual native metadata')
    need(all(sha(r['source'])==r['source_sha256'] for r in rows),'fixture contents preserved');need(all(sha(p)==h for p,h in sources.items()),'source identities after');need(all(sha(p)==h for p,h in protected.items()),'protected identities after')
    result={'status':'VERIFIED bounded original file/header prefix; full image/loading/error publication UNVERIFIED','time_utc':datetime.now(timezone.utc).isoformat(),'previous_goal_turn':'PROGRESS','cases':len(rows),'accepted':accepted,'error_dispatches':len(rows)-accepted,'coverage':coverage,'repetitions':2,'trace_bytes':len(traces[0]),'trace_sha256':hashlib.sha256(traces[0]).hexdigest(),'native_warning_events':warnings_seen,'executable':{'path':str(exe),'sha256':sha(exe),'architecture':'x86 private native reference'},'compiler_warnings':warnings,'authenticated_ranges':[{k:v for k,v in e.items() if k!='instructions'} for e in ranges],'source_sha256':sources,'protected_sha256':protected,'limits':['Actual original VTF factory/constructor/header parser and texture-reader open/retry/read/seek/header-failure control flow run. Filesystem methods map logical read-only requests to actual disposable-file snapshots. GAME path-ID is an explicit supplied result, not measured native mount selection.','Native execution stops at header acceptance or error-construction dispatch, before CTexture data publication, full-image reading, error-resource creation or GPU. Successful prefix leaves the emulated file open; host tears down the disposable scope, not native close lifecycle.','Scratch buffer is supplied 2MiB zero-filled storage with false optimal-IO-constraints response. Truncated-input conclusions apply to these explicit scratch/byte conditions, not all native stale-buffer/malformed-file behavior.','Symbols and warnings are controlled services. New probe is x86-only investigation; no new owned runtime or editor build/GPU run/live SFM/original writes/publication.']}
    result['status']='VERIFIED bounded native file/header prefix and owned x64 flag normalization; full image/loading/error publication UNVERIFIED'
    result.update(owned_flag_cases=flag_cases,owned_executable={'path':str(owned),'sha256':sha(owned),'architecture':'AMD64 PE32+'},owned_compiler_warnings=owned_warnings)
    result['limits'][-1]='Symbols and warnings are controlled services. The new owned helper only normalizes supplied VTF7.1..7.5 flags; raw layout reader, production material state and renderer remain unchanged. No editor build/GPU run/live SFM/original writes/publication.'
    receipt=run/'results.json';receipt.write_text(json.dumps(result,indent=2)+'\n');(OUT/'latest-verification.json').write_text(json.dumps({'receipt':str(receipt),'receipt_sha256':sha(receipt)},indent=2)+'\n');print(json.dumps({k:result[k] for k in ['status','cases','accepted','error_dispatches','coverage','trace_bytes','trace_sha256','native_warning_events','executable','compiler_warnings','owned_flag_cases','owned_executable','owned_compiler_warnings']},indent=2),flush=True)

if __name__=='__main__':main()
