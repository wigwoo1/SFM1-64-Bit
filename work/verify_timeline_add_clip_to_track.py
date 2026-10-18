import hashlib,json,re,sys
from pathlib import Path
sys.path.insert(0,"work")
from pe_image import Image
binary=r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
im=Image(binary); digest=hashlib.sha256(im.b).hexdigest()
prior=json.loads(Path("outputs/timeline-track-context-menu-population-verification.json").read_text(encoding="utf-8-sig")); assert prior["sha256"]==digest
def verify(s,e):
 rows=[]; cur=None; active=False
 for line in Path("work/ifm-disassembly.txt").open(errors="replace"):
  m=re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)",line)
  if m:
   va=int(m.group(1),16)
   if va<s: continue
   active=True
   if cur: rows.append(cur)
   if va>=e: cur=None;break
   cur={"va":va,"b":bytes.fromhex(m.group(2))}
  elif active and cur and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$",line):cur["b"]+=bytes.fromhex(line.strip())
 if cur:rows.append(cur)
 pos=s;n=0;bad=[]
 for r in rows:
  o=im.offset(r["va"]);b=r["b"]
  if r["va"]!=pos or im.b[o:o+len(b)]!=b:bad.append(hex(r["va"]))
  pos=r["va"]+len(b);n+=len(b)
 if pos!=e:bad.append("end="+hex(pos))
 return {"start":hex(s),"end_exclusive":hex(e),"instruction_rows":len(rows),"bytes":n,"all_match":not bad,"mismatches":bad}
spans={"effects_add_clip_track_handler":verify(0x1017CF60,0x1017D23D),"sound_add_clip_track_handler":verify(0x101692A0,0x101693A8),"clip_factory_timing_material_initializer":verify(0x10283D20,0x10283E10),"time_start_field_writer":verify(0x104BBB60,0x104BBB91),"duration_field_writer":verify(0x104BBBA0,0x104BBBF1),"third_timing_property_writer":verify(0x104BBB20,0x104BBB60),"float_timing_property_writer":verify(0x104BC9A0,0x104BC9EA)}
assert all(x["all_match"] for x in spans.values()),spans
strings={hex(a):im.string(a) for a in [0x10AC55CC,0x10AC55BC,0x10AC55B0]}
result={"binary":binary,"sha256":digest,"parent_menu_receipt":"outputs/timeline-track-context-menu-population-verification.json","handler_spans":spans,"strings":strings,"observed_static_flow":["Effects Track callback 0x1017CF60 validates its model/context, enumerates eligible clip-related entries, and reaches a dialog/service path using strings at 0x10AC55CC and 0x10AC55BC.","It supplies two copies of a float constant from 0x10A8A56C to converter 0x1097B170, then calls 0x10283D20 with the selected target and label string at 0x10AC55B0.","Factory helper 0x10283D20 writes the supplied string through 0x10366100 and writes four scalar/timing inputs through typed helper calls 0x104BBB60, 0x104BBBA0, 0x104BBB20 and 0x104BC9A0 before returning the element.","If the returned element passes an interface check against 0x10EB1104, the callback calls 0x104D2FE0 to attach it through a type-checked context collection path.","Sound Track callback 0x101692A0 shares validation/service calls but has a distinct body and additional branch conditions; it is not treated as identical behavior."],"limitations":["The exact strings' dialog roles and the float units are unconfirmed.","The exact schema meaning of the string field and four scalar/timing fields, selected child semantics and context collection schema remain unresolved.","No live SFM menu invocation, session diff, undo, playback or save/reopen test."]}
Path("outputs/timeline-add-clip-to-track-handler-verification.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print(json.dumps({"sha256":digest,"spans":spans,"strings":strings},indent=2))
