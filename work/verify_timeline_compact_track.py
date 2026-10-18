import hashlib, json, re, sys
from pathlib import Path
sys.path.insert(0,"work")
from pe_image import Image
binary=r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
im=Image(binary); digest=hashlib.sha256(im.b).hexdigest()
receipt=json.loads(Path("outputs/timeline-compact-track-action-recovery.json").read_text(encoding="utf-8-sig"))
assert receipt["binary_sha256"]==digest
def verify(start,end):
 rows=[];cur=None;found=False
 for line in Path("work/ifm-disassembly.txt").open(errors="replace"):
  m=re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)",line)
  if m:
   va=int(m.group(1),16)
   if va<start: continue
   found=True
   if cur: rows.append(cur)
   if va>=end: cur=None; break
   cur={"va":va,"b":bytes.fromhex(m.group(2))}
  elif found and cur and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$",line): cur["b"]+=bytes.fromhex(line.strip())
 if cur: rows.append(cur)
 pos=start;count=0;errors=[]
 for row in rows:
  off=im.offset(row["va"]); raw=row["b"]
  if row["va"]!=pos or im.b[off:off+len(raw)]!=raw: errors.append(hex(row["va"]))
  pos=row["va"]+len(raw);count+=len(raw)
 if pos!=end: errors.append("end="+hex(pos))
 return {"start":hex(start),"end_exclusive":hex(end),"instruction_rows":len(rows),"bytes":count,"all_match":not errors,"mismatches":errors}
ranges={"compact_track_handler":(0x1017C8C0,0x1017C9F9),"timing_sum_getter":(0x104BBA80,0x104BBACB),"timing_field_writer":(0x104BBB60,0x104BBB91),"typed_data_model_writer":(0x10366370,0x10366474),"collection_count_accessor":(0x1008C0E0,0x1008C0E4),"action_constructor_binding":(0x101775C4,0x101775F0),"context_menu_wiring_a":(0x1017333F,0x10173357),"context_menu_wiring_b":(0x101735FD,0x10173615)}
spans={k:verify(*v) for k,v in ranges.items()}
assert all(x["all_match"] for x in spans.values()),spans
result={"binary":binary,"sha256":digest,"parent_receipt":"outputs/timeline-compact-track-action-recovery.json","helper_spans":spans,"total_verified_bytes":sum(x["bytes"] for x in spans.values()),"behavior_summary":"For collection count >1, reads element 0 timing sum, then for each later index writes the carried value to a typed timing field, reads that element timing sum, and carries the new value. For count <=1, no repositioning loop. The operation's precise schema names and runtime visible result are not established statically.","limitations":["Static code and registration only; no live SFM execution, document diff, undo, playback, or save/reopen test.","Underlying timing field semantic names/time units and exact user-facing effect remain unidentified."]}
Path("outputs/timeline-compact-track-byte-verification.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print(json.dumps({"sha256":digest,"verified_spans":spans,"total_bytes":result["total_verified_bytes"]},indent=2))
