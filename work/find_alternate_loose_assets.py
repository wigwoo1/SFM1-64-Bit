from pathlib import Path
import os,struct,json,time
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\usermod\models');records=[];scanned=0;started=time.monotonic();limited=False
for folder,dirs,files in os.walk(root):
 for name in files:
  if not name.lower().endswith('.mdl'):continue
  p=Path(folder,name);scanned+=1
  try:
   with p.open('rb') as f:
    h=f.read(240)
    if len(h)<240 or h[:4]!=b'IDST' or struct.unpack_from('<i',h,4)[0]!=49:continue
    families=struct.unpack_from('<i',h,224)[0];count,start=struct.unpack_from('<ii',h,232)
    if count<0 or count>256 or start<0:continue
    f.seek(start);b=f.read(count*16)
    if len(b)!=count*16:continue
    counts=[struct.unpack_from('<i',b,n*16+4)[0] for n in range(count)]
   if families>1 or any(c>1 for c in counts):
    vvd=p.with_suffix('.vvd');vtx=p.with_suffix('.dx90.vtx');fixups=None
    if vvd.exists():
     with vvd.open('rb') as f:h=f.read(64)
     if len(h)==64:fixups=struct.unpack_from('<I',h,48)[0]
    if vtx.exists() and fixups==0:records.append({'model':str(p),'skin_families':families,'body_model_counts':counts})
  except (OSError,struct.error):pass
  if len(records)>=8 or time.monotonic()-started>20:limited=True;break
 if limited:break
report={'scope':str(root),'scanned':scanned,'limited':limited,'eligible_assets':records};Path('outputs/sfm64-alternate-loose-assets.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
