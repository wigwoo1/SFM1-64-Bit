"""Read-only application of statically recovered loader predicate, not execution parity."""
import json,struct,tempfile
from pathlib import Path
from verify_native_convex_brush_ray import ROOT,MAP,sha,require

def main():
 before=sha(MAP);b=MAP.read_bytes()
 def lump(i):return struct.unpack_from('<II',b,8+16*i)
 po,pn=lump(1);so,sn=lump(19);bo,bn=lump(18)
 planes=[struct.unpack_from('<4fi',b,po+i) for i in range(0,pn,20)]
 selected=[];zero=[];inconsistent=[]
 for i in range(bn//12):
  first,count,contents=struct.unpack_from('<iii',b,bo+12*i)
  require(first>=0 and count>=0 and first+count<=sn//8,'Invalid side span')
  types=[planes[struct.unpack_from('<H',b,so+8*s)[0]][4]&255 for s in range(first,first+count)]
  if count==0:zero.append(i)
  if not(count==0 or count==6 and all(t<=2 for t in types)):continue
  selected.append(i);faces=set()
  for s in range(first,first+count):
   p=planes[struct.unpack_from('<H',b,so+8*s)[0]];axis=p[4]&255
   if axis>2 or abs(p[axis])!=1 or any(p[j]!=0 for j in range(3) if j!=axis):inconsistent.append(i);break
   faces.add((axis,p[axis]))
  if len(faces)!=6 and i not in inconsistent:inconsistent.append(i)
 out=ROOT/'outputs/optimized-box-native-20261015';run=Path(tempfile.mkdtemp(prefix='classification-',dir=out))
 receipt=dict(status='STATIC_PREDICATE_APPLIED_NOT_NATIVE_LOADER_EXECUTION',map=str(MAP),map_sha256=before,planes=len(planes),brushes=bn//12,selected_count=len(selected),selected_brushes=selected,zero_side_brushes=zero,inconsistent_axial_face_brushes=inconsistent,limits=['Runtime type copied from low byte of BSP plane type per checked instructions','Constructor and classifier not executed by this inspector','No production behavior changed'])
 require(sha(MAP)==before,'Original map changed');(run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(selected=len(selected),zero=len(zero),inconsistent=len(inconsistent),receipt=str(run/'results.json'))))
if __name__=='__main__':main()
