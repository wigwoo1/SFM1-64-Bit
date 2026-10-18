import io,json,struct,subprocess,hashlib
from pathlib import Path
from PIL import Image
inventory=json.loads(Path('outputs/sfm64-effects-animated-vtf-inventory.json').read_text())
choices=[r for r in inventory['animated'] if r['version'][1] in (2,3,4) and r['format'] in (13,15) and r['bytes']<1048576]
assert choices,'no suitable installed animation'
record=choices[0];p=Path(record['path']);b=p.read_bytes();prefix='outputs/sfm64-real-animation'
r=subprocess.run(['build/session-core/dump_vtf_animation.exe',str(p),prefix],capture_output=True,text=True);assert r.returncode==0,r.stderr
header=struct.unpack_from('<I',b,12)[0];offset=header
if record['version'][1]==2:offset+=((b[61]+3)//4)*((b[62]+3)//4)*8
else:
 resources=struct.unpack_from('<I',b,68)[0]
 offset=next(struct.unpack_from('<I',b,84+i*8)[0] for i in range(resources) if struct.unpack_from('<I',b,80+i*8)[0]==0x30)
comparisons=[]
for mip in reversed(range(record['mips'])):
 w=max(1,record['width']>>mip);h=max(1,record['height']>>mip);size=((w+3)//4)*((h+3)//4)*(8 if record['format']==13 else 16)
 for frame in range(record['frames']):
  payload=b[offset:offset+size];assert len(payload)==size;offset+=size
  dds=bytearray(128);dds[:4]=b'DDS ';struct.pack_into('<7I',dds,4,124,0x81007,h,w,size,0,0);struct.pack_into('<2I',dds,76,32,4);dds[84:88]=b'DXT1' if record['format']==13 else b'DXT5';struct.pack_into('<I',dds,108,0x1000)
  expected=Image.open(io.BytesIO(dds+payload)).convert('RGBA').tobytes();actual=Path(f'{prefix}-m{mip}-f{frame}.rgba').read_bytes();assert len(actual)==len(expected)
  differences=[abs(x-y) for x,y in zip(actual,expected)]
  comparisons.append(dict(mip=mip,frame=frame,rgba_sha256=hashlib.sha256(actual).hexdigest(),differing_components=sum(x!=0 for x in differences),maximum_difference=max(differences)))
assert offset==len(b)
result=dict(asset=record,source_sha256=hashlib.sha256(b).hexdigest(),reference='Pillow DDS decoder; independent VTF slice calculation',comparisons=comparisons)
Path('outputs/sfm64-real-animated-vtf-validation.json').write_text(json.dumps(result,indent=2))
print(r.stdout.strip());print('comparisons',len(comparisons),'maximum difference',max(x['maximum_difference'] for x in comparisons),'unique decoded frames',len(set(x['rgba_sha256'] for x in comparisons)))
