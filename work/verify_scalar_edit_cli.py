from pathlib import Path
import subprocess,struct,math,hashlib,json,uuid
root=Path(__file__).resolve().parent.parent
exe=root/'build/session-core/sfm64-session-core.exe'
fixture=root/'work/roundtrip-11.dmx'
original=fixture.read_bytes()
# Fixture11 expression3743:value wire span previously located by full saved-byte
# comparison; this is a fixture-specific integration check, not a DMX parser.
offset=1113127
directory=root/'outputs'/('scalar-edit-cli-'+uuid.uuid4().hex)
directory.mkdir()
checks=[]
def run(output,element,attribute,value):
 return subprocess.run([str(exe),'edit-scalar',str(fixture),str(output),str(element),attribute,value],cwd=root,capture_output=True,text=True)
for i,value in enumerate(['0.25','-2.5','1e-3','nan']):
 output=directory/f'valid-{i}.dmx';result=run(output,3743,'value',value)
 assert result.returncode==0,(value,result.stderr)
 saved=output.read_bytes();assert len(saved)==len(original)
 assert saved[:offset]==original[:offset] and saved[offset+4:]==original[offset+4:]
 actual=struct.unpack_from('<f',saved,offset)[0]
 if value=='nan':assert math.isnan(actual)
 else:assert saved[offset:offset+4]==struct.pack('<f',float(value))
 prior=saved;rejected=run(output,3743,'value','7')
 assert rejected.returncode!=0 and output.read_bytes()==prior
 checks.append({'value':value,'save':True,'only_target_bytes_changed':True,'overwrite_rejected':True})
for i,(element,attribute,value) in enumerate([(3743,'value','abc'),(3743,'value','1.0junk'),(4294967295,'value','1'),(3743,'expr','1'),(3743,'missing_attribute','1')]):
 output=directory/f'invalid-{i}.dmx';result=run(output,element,attribute,value)
 assert result.returncode!=0 and not output.exists(),(element,attribute,value,result.stdout)
assert fixture.read_bytes()==original
report={'fixture':str(fixture),'target':'expression3743:value','wire_offset':offset,'valid_cases':checks,'invalid_cases':5,'input_unchanged':True,'executable_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'fixture_sha256':hashlib.sha256(original).hexdigest(),'output_directory':str(directory),'limits':'Fixture-specific serialized edit integration. No animation reevaluation, native scheduler or graphical property control verification.'}
(root/'outputs/sfm64-scalar-edit-cli-validation.json').write_text(json.dumps(report,indent=2)+'\n')
print('scalar_edit_cli_valid=4 overwrite_rejections=4 invalid_rejections=5 input_unchanged=true')
