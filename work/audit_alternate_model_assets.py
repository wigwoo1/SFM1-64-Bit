from pathlib import Path
import struct,json
root=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game')
models=list(root.rglob('*.mdl'));records=[];errors=0
for p in models:
 try:
  b=p.read_bytes()
  if len(b)<240 or b[:4]!=b'IDST' or struct.unpack_from('<i',b,4)[0]!=49:continue
  families=struct.unpack_from('<i',b,224)[0];count,start=struct.unpack_from('<ii',b,232)
  if count<0 or count>256 or start<0 or start+count*16>len(b):continue
  counts=[struct.unpack_from('<i',b,start+n*16+4)[0] for n in range(count)]
  if families<=1 and not any(c>1 for c in counts):continue
  vvd=p.with_suffix('.vvd');vtx=p.with_suffix('.dx90.vtx')
  fixups=None
  if vvd.exists():
   with vvd.open('rb') as f:header=f.read(64)
   if len(header)==64:fixups=struct.unpack_from('<I',header,48)[0]
  records.append({'model':str(p),'skin_families':families,'body_model_counts':counts,'vvd_present':vvd.exists(),'vtx_present':vtx.exists(),'vvd_fixups':fixups})
 except (OSError,ValueError,struct.error):errors+=1
report={'mdl_files_scanned':len(models),'errors':errors,'alternate_assets':records}
Path('outputs/sfm64-alternate-model-asset-audit.json').write_text(json.dumps(report,indent=2)+'\n')
eligible=[r for r in records if r['vvd_present'] and r['vtx_present'] and r['vvd_fixups']==0]
print(json.dumps({'scanned':len(models),'alternate_assets':len(records),'eligible_current_geometry_reader':len(eligible),'examples':eligible[:8]},indent=2))
