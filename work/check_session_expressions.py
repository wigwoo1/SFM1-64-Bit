"""Read-only corpus evaluation of serialized expression operator inputs."""
import concurrent.futures,hashlib,json,pathlib,re,struct,subprocess
root=pathlib.Path(__file__).resolve().parent.parent
records=json.loads((root/'outputs/sfm-native-table-validation.json').read_text())
paths=[r['path'] for r in records if r['exit_code']==0]
executable=root/'work/inspect_session_expressions.exe'
def digest(path):
 h=hashlib.sha256()
 with open(path,'rb') as f:
  for b in iter(lambda:f.read(1048576),b''):h.update(b)
 return h.hexdigest()
def check(path):
 before=digest(path)
 run=subprocess.run([str(executable),path],capture_output=True,text=True,timeout=120)
 after=digest(path)
 record=dict(path=path,exit_code=run.returncode,unchanged=before==after,sha256=after)
 if run.returncode:return dict(record,error=run.stderr[:2000])
 lines=run.stdout.splitlines();summary=re.fullmatch(r'operators=(\d+) parsed=(\d+) complete=(\d+)',lines[-1])
 if not summary:raise ValueError('Missing inspector summary')
 operators,parsed,complete=map(int,summary.groups());matches=0;mismatches=[];incomplete=[];formulas={}
 for line in lines[:-1]:
  expression=re.search(r' expr=(.*?) variables=',line).group(1)
  formulas[expression]=formulas.get(expression,0)+1
  pair=re.search(r' result=(\S+) stored=(\S+)',line)
  if pair:
   actual,stored=(struct.pack('<f',float(x)) for x in pair.groups())
   if actual==stored:matches+=1
   else:mismatches.append(line)
  else:raise ValueError('Missing stored result: '+line)
  if ' complete=0 ' in line:incomplete.append(line)
 return dict(record,operators=operators,parsed=parsed,complete=complete,matching=matches,
             mismatching=len(mismatches),mismatch_examples=mismatches[:10],
             incomplete_examples=incomplete[:10],formulas=formulas)
results=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
 for result in pool.map(check,paths):
  results.append(result)
  if len(results)%50==0:print(f'Checked {len(results)}/{len(paths)} sessions',flush=True)
summary=dict(sessions=len(results),successful=sum(r['exit_code']==0 for r in results),
 unchanged=all(r['unchanged'] for r in results))
for field in ['operators','parsed','complete','matching','mismatching']:summary[field]=sum(r.get(field,0) for r in results)
(root/'outputs/sfm-session-expression-corpus.json').write_text(json.dumps(dict(summary=summary,sessions=results),indent=2))
print(json.dumps(summary),flush=True)
