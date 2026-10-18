from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if match:
  if pending:rows.append(pending)
  pending={'address':int(match[1],16),'bytes':bytes.fromhex(match[2]),'instruction':match[3].strip()}
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line)
if pending:rows.append(pending)
begin=0x1002f410;selected=[]
for row in rows:
 if row['address']<begin:continue
 selected.append(row)
 if row['instruction'].startswith('ret'):break
assert selected[0]['address']==begin
end=selected[-1]['address']+len(selected[-1]['bytes']);cursor=begin;calls=[];branches=[]
starts={r['address'] for r in selected}
for r in selected:
 a,b=r['address'],r['bytes'];assert a==cursor;cursor+=len(b)
 assert im.b[im.offset(a):im.offset(a)+len(b)]==b
 if b[0]==0xe8:
  assert len(b)==5;calls.append({'address':hex(a),'target':hex(a+5+int.from_bytes(b[1:],'little',signed=True))})
 if r['instruction'].startswith('j'):
  target=int(r['instruction'].split()[1],16);assert target in starts
  branches.append({'address':hex(a),'target':hex(target)})
context=json.loads(Path('outputs/sfm64-full-restore-dependency-audit.json').read_text())
callsite=[c for c in context['direct_calls'] if c['target']==hex(begin)];assert len(callsite)==1
report={'sha256':sha,'begin':hex(begin),'end':hex(end),'verified_bytes':end-begin,'instructions':[{'address':hex(r['address']),'bytes':r['bytes'].hex(),'instruction':r['instruction']} for r in selected],'direct_calls':calls,'branches':branches,'full_restore_callsite':callsite,
 'observed_prefix':'Receiver fields bytefc=0, DWORD100=2,104=1,108=1. First command is opcode1/state27/value0 or device slot57 equivalent. Each following command reacquires command state through1000df50; actual wholefunction semantics still to map.',
 'limits':'Static instruction and branch verification only. No original execution or x64 wholehelper implementation yet. First return bounds selected; all branch targets checked internal.'}
Path('outputs/sfm64-restore-blend-helper-recovery.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({'verified_bytes':end-begin,'end':hex(end),'calls':calls,'branches':len(branches),'callsite':callsite[0]['context']}))
