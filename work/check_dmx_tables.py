import json,pathlib,subprocess
root=pathlib.Path(__file__).resolve().parent.parent
fixtures=json.loads((root/'outputs'/'sfm-session-fixtures.json').read_text())
results=[]
for fixture in fixtures:
 if fixture['header']=='unrecognized':continue
 p=subprocess.run([str(root/'work'/'dmx_tables.exe'),fixture['path']],capture_output=True,text=True)
 results.append(dict(path=fixture['path'],exit_code=p.returncode,result=json.loads(p.stdout) if p.returncode==0 else p.stderr.strip()))
(root/'outputs'/'sfm-native-table-validation.json').write_text(json.dumps(results,indent=2))
print('Passed:',sum(r['exit_code']==0 for r in results),'Failed:',sum(r['exit_code']!=0 for r in results))
