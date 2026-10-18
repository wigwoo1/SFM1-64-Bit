from pathlib import Path
import hashlib,json,re
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
tables={'ordinary':[im.word(0x1016e13c+4*i) for i in range(13)],'packed':[im.word(0x1016e170+4*i) for i in range(2)]}
rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  if pending:rows.append(pending)
  pending={'address':int(m[1],16),'bytes':bytes.fromhex(m[2]),'instruction':m[3].strip()}
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line)
if pending:rows.append(pending)
results=[]
for begin in [0x1002e1f0,0x1002e510,0x1002e580,0x1002e700,0x1002e8e0]:
 selected=[];required=begin
 for row in rows:
  a=row['address']
  if a<begin:continue
  assert a-begin<4096,'Handler extent requires manual boundary review'
  selected.append(row)
  if row['instruction'].startswith('j'):
   operand=row['instruction'].split()[1]
   if re.fullmatch('[0-9A-F]{8}',operand):
    target=int(operand,16);assert target>=begin,'External tail branch requires review';required=max(required,target)
  if row['instruction'].startswith('ret') and a>=required:break
 assert selected[0]['address']==begin and selected[-1]['instruction'].startswith('ret')
 cursor=begin;starts={r['address'] for r in selected};calls=[]
 for r in selected:
  a,b=r['address'],r['bytes'];assert a==cursor;cursor+=len(b);assert im.b[im.offset(a):im.offset(a)+len(b)]==b
  if b[0]==0xe8:assert len(b)==5;calls.append(hex(a+5+int.from_bytes(b[1:],'little',signed=True)))
  if r['instruction'].startswith('j'):
   operand=r['instruction'].split()[1]
   if re.fullmatch('[0-9A-F]{8}',operand):assert int(operand,16) in starts
 results.append({'begin':hex(begin),'end':hex(cursor),'bytes':cursor-begin,'direct_targets':calls,'instructions':[{'address':hex(r['address']),'bytes':r['bytes'].hex(),'instruction':r['instruction']} for r in selected]})
out={'sha256':sha,'tables':{k:[hex(a) for a in v] for k,v in tables.items()},'handlers':results,'wrapper_bindings':{'0':'0x1002e510','3':'0x1002e580','5':'0x1002e1f0','6':'0x1002e700','7':'0x1002e8e0'},'limits':'Exact handler bodies and branch coverage verified. No original execution or x64 handler semantics inferred solely from table membership. Indirect dependencies remain unresolved.'}
Path('outputs/sfm64-transition-wrapper-helper-recovery.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps({'unique_handlers':len(results),'verified_bytes':sum(r['bytes'] for r in results),'direct_targets':{r['begin']:r['direct_targets'] for r in results}}))


