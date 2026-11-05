"""Rebuild owned inspector and authenticate static output-context evidence only."""
from pathlib import Path
from datetime import datetime,timezone
import hashlib,json,re,struct,subprocess,tempfile
from audit_installed_output_context import audit
ROOT=Path(__file__).resolve().parent.parent
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def need(x,s):
    if not x:raise RuntimeError(s)
def main():
    parent=Path((ROOT/'outputs/native-output-context-current.txt').read_text().strip());before=json.loads((parent/'before.json').read_text());run=Path(tempfile.mkdtemp(prefix='run-',dir=parent));r=dict(status='IN PROGRESS',time_utc=datetime.now(timezone.utc).isoformat(),preceding_goal_turn='PROGRESS: explicit HDR2 editor GPU route verified',run=str(run),commands=[])
    game=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game');source=game/'tf_movies/elements/sessions/mtt_soldier/mtt_soldier.dmx'
    def execute(args,label,expected=0):
        args=list(map(str,args));p=subprocess.run(args,cwd=ROOT,capture_output=True,text=True,timeout=120,creationflags=subprocess.CREATE_NO_WINDOW);(run/(label+'.log')).write_text(p.stdout+p.stderr,encoding='utf-8');r['commands'].append(dict(args=args,exit_code=p.returncode,expected=expected));need(p.returncode==expected,label+' failed: '+(p.stdout+p.stderr)[-1200:]);return p.stdout+p.stderr
    try:
        need(all(sha(ROOT/p)==h for p,h in before.items()),'protected initial source/build identity')
        originals=[source,game/'tf_movies/maps/mtt_soldier.bsp',game/'usermod/gameinfo.txt',game/'bin/tools/ifm.dll',game/'bin/engine.dll',game/'bin/materialsystem.dll',game/'bin/shaderapidx9.dll',game/'tf/bin/client.dll']
        pins={str(p):sha(p) for p in originals};static=audit(run/'static')
        need(static['instruction_count']==414,'complete static instruction coverage');r['static']=dict(path=str(run/'static/evidence.json'),sha256=sha(run/'static/evidence.json'),instructions=414)
        out=run/'owned-x64';execute(['cmd.exe','/d','/c',ROOT/'work/build_saved_tone_map_inspector.cmd',out],'build')
        exe=out/'inspect_saved_tone_map_scale_x64.exe';b=exe.read_bytes();i=struct.unpack_from('<I',b,60)[0];need((struct.unpack_from('<H',b,i+4)[0],struct.unpack_from('<H',b,i+24)[0])==(0x8664,0x20b),'owned AMD64 PE32+')
        warnings=re.findall(r'warning (C\d+): ([^\r\n]+)',(out/'build.log').read_text(errors='replace'));need(not warnings,'unexpected inspector warnings')
        u=lambda n:struct.pack('<I',n);strings=[b'DmElement',b'renderSettings',b'toneMapScale',b'DmeCamera',b'camera_test'];prefix=b'<!-- dmx encoding binary 5 format sfm_session 20 -->\0'+u(5)+b''.join(s+b'\0' for s in strings)+u(2)+u(0)+u(1)+bytes([1])*16+u(3)+u(4)+bytes([2])*16
        payload=u(1)+u(2)+bytes([3])+struct.pack('<f',.5)+u(1)+u(2)+bytes([3])+struct.pack('<f',2)
        fixture=run/'owned.dmx';fixture.write_bytes(prefix+payload);text=execute([exe,fixture],'owned')
        need('saved_scale=0.5 bits=0x3f000000' in text and 'saved_scale=2 bits=0x40000000' in text and 'cameras=1 attributes=2' in text,'independent literal saved values')
        bad=run/'bad-schema.dmx';bad.write_bytes(prefix+u(1)+u(2)+bytes([2])+struct.pack('<f',.5)+payload[13:]);bad_before=sha(bad)
        text=execute([exe,bad],'bad-schema',1);need('toneMapScale saved scalar schema' in text and sha(bad)==bad_before,'bad schema read-only refusal')
        logs=[execute([exe,source],'benchmark-'+str(n)) for n in range(2)];need(logs[0]==logs[1],'repeated saved-value inspection')
        need('element=13141 type="DmElement" name="renderSettings" saved_scale=1 bits=0x3f800000' in logs[0] and 'element=13180 type="DmeCamera" name="camera100" saved_scale=1 bits=0x3f800000' in logs[0] and 'cameras=41 attributes=42' in logs[0],'benchmark saved metadata coverage')
        need(fixture.read_bytes()==prefix+payload and all(sha(p)==h for p,h in pins.items()),'original/owned inputs changed')
        need(all(sha(ROOT/p)==h for p,h in before.items()),'protected final source/build identity')
        r.update(status='PARTIALLY VERIFIED native output; bounded static context and saved metadata VERIFIED',
            inspector=dict(path=str(exe),sha256=sha(exe),architecture='AMD64 PE32+',warnings=warnings),benchmark_output=logs[0],
            original_sha256=pins,protected_sha256=before,source_sha256={n:sha(ROOT/'work'/n) for n in ['audit_installed_output_context.py','inspect_saved_tone_map_scale_x64.cpp','build_saved_tone_map_inspector.cmd','verify_installed_output_context.py']},
            limits=['No original code/process execution; all installed DLL evidence is static file analysis.',
                    'Saved values only, not evaluated camera animation or original live settings.',
                    'Actual target allocation/format and final Engine_Post selected shader/draw chain remain UNVERIFIED.',
                    'No editor dependency changed; no new editor build/GPU runtime result claimed by this audit.'])
    except Exception as error:r.update(status='PARTIALLY VERIFIED',failure=repr(error));raise
    finally:
        p=run/'results.json';p.write_text(json.dumps(r,indent=2)+'\n');(parent/'latest.json').write_text(json.dumps(dict(receipt=str(p),sha256=sha(p)),indent=2)+'\n')
    print(json.dumps(dict(status=r['status'],receipt=str(p),inspector=r['inspector'],protected=len(before)),indent=2))
if __name__=='__main__':main()
