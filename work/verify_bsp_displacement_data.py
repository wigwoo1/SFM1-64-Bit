import collections,json,struct,subprocess,tempfile
from pathlib import Path
from verify_native_convex_brush_ray import ROOT,MAP,cube,sha,require
def main():
 out=ROOT/'outputs/bsp-displacement-data-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));exe=out/'displacement_data_x64.exe';binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==0x8664,'Wrong architecture');before=sha(MAP);fixture=cube()
 def append(b,id,payload):struct.pack_into('<IIII',b,8+id*16,len(b),len(payload),0,0);b.extend(payload)
 info=bytearray(176);struct.pack_into('<i',info,20,2);struct.pack_into('<I',info,32,1);append(fixture,26,info);append(fixture,33,b''.join(struct.pack('<5f',0,0,1,i*.01,0) for i in range(25)));append(fixture,48,b''.join(struct.pack('<H',i) for i in range(32)));controlled=run/'controlled.bsp';controlled.write_bytes(fixture);datasets=[]
 def expected(b):
  def lump(id):return struct.unpack_from('<II',b,8+id*16)
  io,il=lump(26);vo,vl=lump(33);to,tl=lump(48);wanted=bytearray(struct.pack('<I',il//176));powers=collections.Counter();contents=collections.Counter()
  for i in range(il//176):
   p=io+176*i;v,t,power=struct.unpack_from('<IIi',b,p+12);width=(1<<power)+1;vc=width*width;tc=2*(width-1)**2;wanted.extend(b[p:p+176]);wanted.extend(b[vo+20*v:vo+20*(v+vc)]);wanted.extend(b[to+2*t:to+2*(t+tc)]);powers[power]+=1;contents[struct.unpack_from('<I',b,p+32)[0]]+=1
  return wanted,dict(records=il//176,vertex_records=vl//20,triangle_tags=tl//2,powers=dict(powers),contents=dict(contents))
 for i,path in enumerate([controlled,MAP]):
  target=run/f'{i}.bin';p=subprocess.run([str(exe),str(path),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);want,inventory=expected(path.read_bytes());require(target.read_bytes()==want,'Raw displacement records/spans differ');datasets.append(dict(map=str(path),inventory=inventory,output_sha256=sha(target)))
 def offset(b,id):return struct.unpack_from('<I',b,8+id*16)[0]
 rejected=[]
 mutations=[('power',lambda b:struct.pack_into('<i',b,offset(b,26)+20,31)),('vertex-span',lambda b:struct.pack_into('<I',b,offset(b,26)+12,100)),('triangle-span',lambda b:struct.pack_into('<I',b,offset(b,26)+16,100)),('nan-start',lambda b:struct.pack_into('<I',b,offset(b,26),0x7fc00000)),('nan-vertex',lambda b:struct.pack_into('<I',b,offset(b,33),0x7fc00000)),('stride',lambda b:struct.pack_into('<I',b,8+33*16+4,21)),('version',lambda b:struct.pack_into('<I',b,8+48*16+8,1)),('outside',lambda b:struct.pack_into('<I',b,8+33*16,0xfffffff0))]
 for name,mutate in mutations:
  b=bytearray(fixture);mutate(b);source=run/(name+'.bsp');source.write_bytes(b);target=run/(name+'.bin');p=subprocess.run([str(exe),str(source),str(target)],capture_output=True,text=True,timeout=10);require(p.returncode!=0 and not target.exists(),'Unsafe displacement input accepted: '+name);rejected.append(name)
 require(sha(MAP)==before,'Original map changed');receipt=dict(status='PASS_CHECKED_DISPLACEMENT_SOURCE_DATA',executable=str(exe),sha256=sha(exe),datasets=datasets,rejected=rejected,map_sha256=before,limits=['Source records only; no native collision mesh or leaf mapping','Map-face identity/neighbors not interpreted','No collision or editor integration']);(run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],benchmark=datasets[1]['inventory'],rejected=len(rejected),receipt=str(run/'results.json'))))
if __name__=='__main__':main()
