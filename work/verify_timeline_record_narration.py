import hashlib,json,re,sys
from pathlib import Path
sys.path.insert(0,"work")
from pe_image import Image
binary=r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
im=Image(binary);digest=hashlib.sha256(im.b).hexdigest()
menu=json.loads(Path("outputs/timeline-track-context-menu-population-verification.json").read_text(encoding="utf-8-sig"));assert menu["sha256"]==digest
def verify(s,e):
 rows=[];cur=None;active=False
 for line in Path("work/ifm-disassembly.txt").open(errors="replace"):
  m=re.match(r"^\s+([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)",line)
  if m:
   va=int(m.group(1),16)
   if va<s:continue
   active=True
   if cur:rows.append(cur)
   if va>=e:cur=None;break
   cur={"va":va,"b":bytes.fromhex(m.group(2))}
  elif active and cur and re.match(r"^\s{10,}[0-9A-F]{2}(?:\s+[0-9A-F]{2})*\s*$",line):cur["b"]+=bytes.fromhex(line.strip())
 if cur:rows.append(cur)
 p=s;n=0;bad=[]
 for row in rows:
  off=im.offset(row["va"]);b=row["b"]
  if row["va"]!=p or im.b[off:off+len(b)]!=b:bad.append(hex(row["va"]))
  p=row["va"]+len(b);n+=len(b)
 if p!=e:bad.append("end="+hex(p))
 return {"start":hex(s),"end_exclusive":hex(e),"instruction_rows":len(rows),"bytes":n,"all_match":not bad,"mismatches":bad}
ranges={"record_narration_callback":(0x1029AF10,0x1029B0A0),"narration_stop_cleanup_callback":(0x1029A900,0x1029A941),"record_narration_menu_dispatch_wrapper":(0x1016ECE0,0x1016ED71)}
spans={k:verify(*v) for k,v in ranges.items()};assert all(v["all_match"] for v in spans.values()),spans
strings={hex(a):im.string(a) for a in [0x10AD3680,0x10ADC060,0x10AE52F0,0x10AE52D4,0x10AE52B8,0x10A946E8]}
result={"binary":binary,"sha256":digest,"parent_menu_receipt":"outputs/timeline-track-context-menu-population-verification.json","helper_spans":spans,"strings":strings,"observed_static_behavior":["Sound Track menu builder registers Record Narration and routes it through binding helper 0x1016ECE0 to callback 0x1029AF10.","The callback requires a non-null target and uses target state at +0x444 to maintain a narration-file sequence. It formats paths under sound/%s and searches numbered narration%04i.wav filenames, advancing a per-object counter. If the counter reaches 10000, it logs an error and returns without continuing setup.","For an available filename, it initializes a recording-related structure with the target, mode value 2, output path and state fields, then logs Starting narration into %s and calls a global application/service virtual method with media-format token MOD.","If its incoming boolean is set, it invokes 0x1029A900. That routine clears a recording-state byte at +0x69C, conditionally requests a stop/control operation based on internal flags, then tail-calls a separate global cleanup/service method."],"limitations":["No audio device was opened; no WAV was created and no recording/playback, DMX diff, undo or save/reopen test was performed.","The engine service vtable methods, recording synchronization and actual capture/file-writing behavior require deeper tracing before claiming end-to-end narration support.","Live menu behavior and binding argument source remain untested."]}
Path("outputs/timeline-record-narration-handler-verification.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print(json.dumps({"sha256":digest,"spans":spans,"strings":strings},indent=2))
