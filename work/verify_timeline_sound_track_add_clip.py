import hashlib,json,re,sys
from pathlib import Path
sys.path.insert(0,"work")
from pe_image import Image
binary=r"F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll"
im=Image(binary);digest=hashlib.sha256(im.b).hexdigest()
menu=json.loads(Path("outputs/timeline-track-context-menu-population-verification.json").read_text(encoding="utf-8-sig"));assert menu["sha256"]==digest
methods=json.loads(Path("outputs/timeline-editor-method-handler-recovery.json").read_text(encoding="utf-8-sig"));assert any(m.get("signature")=="AddSoundClipToTrackWithDialogAccepted()" and m.get("handler")=="0x1016aa00" for m in methods["methods"])
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
 pos=s;n=0;bad=[]
 for r in rows:
  o=im.offset(r["va"]);b=r["b"]
  if r["va"]!=pos or im.b[o:o+len(b)]!=b:bad.append(hex(r["va"]))
  pos=r["va"]+len(b);n+=len(b)
 if pos!=e:bad.append("end="+hex(pos))
 return {"start":hex(s),"end_exclusive":hex(e),"instruction_rows":len(rows),"bytes":n,"all_match":not bad,"mismatches":bad}
ranges={"sound_track_add_clip_callback":(0x101692A0,0x101693A8),"accepted_slot_handler":(0x1016AA00,0x1016AB2C),"sound_clip_factory_and_attachment":(0x1027D740,0x1027DB61),"clip_data_attach_helper":(0x1027B410,0x1027B470)}
spans={k:verify(*v) for k,v in ranges.items()};assert all(v["all_match"] for v in spans.values()),spans
strings={hex(a):im.string(a) for a in [0x10AC3028,0x10AE41C4,0x10AC5D6C,0x10AC0740,0x10AA5CCC,0x10AA7BD4]}
result={"binary":binary,"sha256":digest,"parent_menu_receipt":"outputs/timeline-track-context-menu-population-verification.json","qt_method_receipt":"outputs/timeline-editor-method-handler-recovery.json","helper_spans":spans,"strings":strings,"observed_static_behavior":["Sound Track Add Clip to Track opens/reuses a cached Add Sound Clip(s) dialog, connects accepted and rejected signals to distinct CQTimelineEditor methods, and retains the target track reference.","The accepted slot obtains the dialog item count and starting timeline value, iterates selected entries, and retrieves each selected path through a dialog virtual method. For each non-null selected item it calls a media/clip creation routine with the target track, selected path and accumulated start time.","The creation routine loads the chosen audio through the media service and reads a duration value, converts it to the engine time representation, creates a clip model record through a data-model factory, writes path/string and timing/scalar fields, then attaches the record through the typed context collection helper.","The accepted slot reads the created clip duration and advances the accumulated start for the next selected item, clears the retained target reference, and issues timeline update notifications. The dialog rejected slot is separately mapped to 0x10169270.","The type string CDmeSoundClip is consistent with a native Source sound clip representation; data is written through Source data-model helpers."],"limitations":["Exact attribute names for the clip source path, duration, start and auxiliary scalar fields are not fully mapped here.","Static flow establishes selected-file processing and model insertion, not playable audio in a live session, channel-format edge cases, waveform display, undo, persistence or save/reopen.","No audio file was imported and no live SFM UI was operated."]}
Path("outputs/timeline-sound-track-add-clip-accepted-flow-verification.json").write_text(json.dumps(result,indent=2)+"\n",encoding="utf-8")
print(json.dumps({"sha256":digest,"spans":spans,"strings":strings},indent=2))
