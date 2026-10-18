from pathlib import Path
import re,json,hashlib
from pe_image import Image
records=[];provider={}
for source,path,ranges in [('work/client-document-gate-disassembly.txt',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf\bin\client.dll',[(0x100a6740,0x100a6750),(0x10293570,0x1029359c),(0x102935a0,0x102935b1),(0x102935c0,0x102935d1),(0x102935e0,0x102935f1),(0x1028fd30,0x1028fd36),(0x104a2420,0x104a2435)]),('work/engine-disassembly.txt',r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll',[(0x10311810,0x10311825),(0x101e66b0,0x101e66b6)])]:
 im=Image(path);rows=[];pending=None
 if 'engine-disassembly' in source:
  ranges += [(0x101e72c0,0x101e72e5),(0x101e72f0,0x101e7315),(0x101e7320,0x101e7345),(0x101e6750,0x101e6755),(0x101e67b0,0x101e67b5),(0x101e6810,0x101e6815)]
  ranges += [(0x101e66a0,0x101e66a6),(0x101e7c50,0x101e7e47)]
  table=im.word(0x1040c3b8);provider={'interface_name':im.string(0x10366d50),'factory':'0x101e66b0','object':'0x1040c3b8','vtable':hex(table),'first_four_slots':[hex(im.word(table+4*n)) for n in range(4)]}
  provider['module_tool_interface']=im.string(0x10366a30) if 'engine-disassembly' in source else provider.get('module_tool_interface')
 for line in Path(source).open():
  m=re.match(r'^\s*([0-9A-F]{8}): ((?:[0-9A-F]{2} )+)(.*)',line)
  if m:
   va=int(m[1],16);pending=None
   if any(a<=va<z for a,z in ranges):pending={'address':hex(va),'bytes':m[2].strip(),'instruction':m[3].strip()};rows.append(pending)
  elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+\n?',line):pending['bytes']+=' '+line.strip()
 for row in rows:
  b=bytes.fromhex(row['bytes']);o=im.offset(int(row['address'],16));assert im.b[o:o+len(b)]==b
 records.append({'source':source,'sha256':hashlib.sha256(im.b).hexdigest(),'instructions':rows})
report={'checked_instructions':sum(len(r['instructions']) for r in records),'engine_provider':provider,'client_binding':'10293570 obtains factory106c4aac through100a6740, requests VCLIENTENGINETOOLS001 and stores result this+4; three null-guarded forwarding methods dispatch slots1,2,3. VCLIENTTOOLS001 separately registers factory1028fd30 returning107a9e4c.','limitations':'Static binding and dispatch; forwarder owner lifecycle and provider method semantics remain unresolved. Do not equate client interface layout with CEngineTool.','records':records}
report['provider_dispatch']={'list':'0x1040c380','signed_count':'0x1040c38c','provider_slot_to_target_slot':{'1':20,'2':21,'3':22},'operation':'Capture count once; if signed count>0, traverse list in ascending order, reload list pointer on each iteration, dispatch receiver virtual callback.','limitations':'Callback target identity, list mutation/lifetime and event names remain unproved; these methods do not directly store CEngineTool timing flag.'}
report['list_population']={'manager':'0x1040c37c','loader':'0x101e7c50','operation':'Module factory requests module_tool_interface; calls its slot0 with1066ca74, requires slot3 result1, invokes slot8, then enumerates slot9 count/slot10 indexed tool pointers. Nonnull tool pointers appended to manager+4 vector, count manager+10.','limitations':'Partial loader recovered through first-vector append only. Helper module loading, concrete IFM dictionary/tools and complete failure/cleanup lifecycle require further proof.'}
Path('outputs/sfm64-client-document-tool-binding.json').write_text(json.dumps(report,indent=2));print('checked_instructions',report['checked_instructions']);print(provider)
