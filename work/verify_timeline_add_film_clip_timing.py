import hashlib,json,re,struct,sys
from pathlib import Path
sys.path.insert(0,"work")
from pe_image import Image
binary=r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"; im=Image(binary); digest=hashlib.sha256(im.b).hexdigest()
prior=json.loads(Path("outputs/timeline-add-film-clip-handler-recovery.json").read_text(encoding="utf-8-sig"))
assert prior["binary_sha256"]==digest and prior["verification_status"]=="Directly byte-verified against installed DLL."
def verify(start,end):
 rows=[];cur=None;found=False
 for line in Path("work/ifm-disassembly.txt").open(errors="replace"):
  m=re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)",line)
  if m:
   va=int(m.group(1),16)
   if va<start:continue
   found=True
   if cur:rows.append(cur)
   if va>=end:cur=None;break
   cur={"va":va,"b":bytes.fromhex(m.group(2))}
  elif found and cur and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$",line):cur["b"]+=bytes.fromhex(line.strip())
 if cur:rows.append(cur)
 cursor=start;count=0;errs=[]
 for row in rows:
  off=im.offset(row["va"]);raw=row["b"]
  if row["va"]!=cursor or im.b[off:off+len(raw)]!=raw:errs.append(hex(row["va"]))
  cursor=row["va"]+len(raw);count+=len(raw)
 if cursor!=end:errs.append("end="+hex(cursor))
 return {"start":hex(start),"end_exclusive":hex(end),"instruction_rows":len(rows),"bytes":count,"all_match":not errs,"mismatches":errs}
ranges={"collection_count_accessor":(0x1008C0E0,0x1008C0E4),"last_clip_interface_resolver":(0x104D31C0,0x104D3207),"clip_end_time_getter":(0x104BBA80,0x104BBACB),"clip_start_time_writer":(0x104BBB60,0x104BBB91),"clip_duration_time_writer":(0x104BBBA0,0x104BBBF0),"seconds_to_engine_time_converter":(0x1097B1E0,0x1097B254)}
spans={k:verify(*v) for k,v in ranges.items()}
const_addrs=[0x10A8BFA0,0x10DDA6D8,0x10A8BBA0]
constants={hex(a):struct.unpack_from("<d",im.b,im.offset(a))[0] for a in const_addrs}
assert all(x["all_match"] for x in spans.values()) and constants[hex(0x10A8BFA0)]==10.0 and constants[hex(0x10DDA6D8)]==10000.0
constant_bytes={hex(a):im.b[im.offset(a):im.offset(a)+8].hex() for a in const_addrs}
result={"binary":binary,"sha256":digest,"parent_receipt":"outputs/timeline-add-film-clip-handler-recovery.json","callback_va":"0x1016C590","helper_spans":spans,"constants":constants,"constant_bytes":constant_bytes,"derived_timing_behavior":["The callback reads the collection count through byte-verified accessor 0x1008C0E0. For a non-empty collection it resolves index count-1 with 0x104D31C0, queries that clip timing record, and obtains start+duration through 0x104BBA80. The empty path supplies zero as the start.","It passes the resolved start value to 0x104BBB60, which writes through the timing interface field at offset +0x60.","The callback converts literal 10.0 seconds using 0x1097B1E0. The converter multiplies by the verified 10000.0 ticks-per-second constant and applies rounding, yielding 100000 ticks.","It passes 100000 to 0x104BBBA0, which writes through timing interface field +0x68. Static control/data flow indicates new clips start at zero or the current final clip end and receive a default 10-second duration."],"limitations":["Underlying interface field names and timebase semantics beyond the observed conversion are not named from static bytes alone.","Timeline insertion ordering, overlap policy, transaction/undo and save/reopen behavior remain untested in a live SFM session.","No live SFM menu/dialog or final clip result was captured."]}
Path("outputs/timeline-add-film-clip-timing-verification.json").write_text(json.dumps(result,indent=2,ensure_ascii=False)+"\n")
print(json.dumps({"sha256":digest,"spans":spans,"constants":constants},indent=2))
