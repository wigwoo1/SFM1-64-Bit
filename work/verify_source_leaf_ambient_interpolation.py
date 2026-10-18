"""Independent raw-BSP float32 reference; no installed engine code executed."""
from pathlib import Path
import hashlib
import json
import math
import struct
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/leaf-ambient-interpolation-20261015'
EXE=OUT/'probe_source_leaf_ambient_interpolation.exe'
MAP=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\maps\mtt_soldier.bsp')
def f(x): return struct.unpack('<f',struct.pack('<f',x))[0]
def bits(x): return struct.unpack('<I',struct.pack('<f',x))[0]
def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok: raise RuntimeError(message)
def reference(b,encoding,leaf,point):
 def lump(i): return struct.unpack_from('<II',b,8+16*i)
 lo,_=lump(10);io,_=lump(51 if encoding=='hdr' else 52);so,_=lump(55 if encoding=='hdr' else 56)
 count,first=struct.unpack_from('<HH',b,io+4*leaf)
 if count==0 and first!=0:
  leaf=first;count,first=struct.unpack_from('<HH',b,io+4*leaf)
 result=[0.0]*18;total=0.0
 if count==0:return [0]*18
 bounds=struct.unpack_from('<6h',b,lo+32*leaf+8)
 for s in range(first,first+count):
  raw=b[so+28*s:so+28*s+28];delta=[]
  for axis in range(3):
   center=f(f(bounds[axis]+bounds[axis+3])*0.5)
   half=f(bounds[axis+3]-center)
   position=f(f(f(raw[24+axis]*half)*f(2/255))+f(center-half))
   delta.append(f(position-f(point[axis])))
  distance=f(f(f(delta[0]*delta[0])+f(delta[1]*delta[1]))+f(delta[2]*delta[2]))
  weight=f(1/f(distance+1));total=f(weight+total)
  for face in range(6):
   exponent=struct.unpack('b',raw[face*4+3:face*4+4])[0]
   table=f(math.ldexp(1.0,exponent)/255)
   for channel in range(3):
    color=f(f(raw[face*4+channel]*table)*255)
    i=face*3+channel;result[i]=f(result[i]+f(color*weight))
 return [bits(f(x*f(1/total))) for x in result]
def fixture():
 b=bytearray(1036);struct.pack_into('<II',b,0,0x50534256,20)
 def append(i,version,payload):
  struct.pack_into('<IIII',b,8+16*i,len(b),len(payload),version,0);b.extend(payload)
 leaves=bytearray(160)
 for i in range(5):struct.pack_into('<6h',leaves,32*i+8,-19,-301,7,238,112,514)
 append(10,1,leaves)
 # leaf1 -> leaf2 -> leaf0: native follows one step only and skips zero.
 index=b''.join(struct.pack('<HH',*v) for v in [(2,0),(0,2),(0,0),(0,4),(1,2)])
 append(51,0,index);append(52,0,index)
 samples=bytearray()
 for i,pos in enumerate([(0,0,0),(255,255,255),(127,63,211)]):
  for face in range(6):samples.extend(bytes([10+i*23+face,61+face,127-face,(-8+i)%256]))
  samples.extend(bytes([*pos,0]))
 append(55,1,samples);append(56,1,bytes(len(samples)))
 return b
def main():
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT))
 b=fixture();path=run/'controlled.bsp';path.write_bytes(b)
 before=digest(MAP);exe_bytes=EXE.read_bytes();pe=struct.unpack_from('<I',exe_bytes,60)[0]
 require(struct.unpack_from('<H',exe_bytes,pe+4)[0]==0x8664,'Not AMD64')
 records=[]
 def check(path,b,encoding,leaf,point,label):
  p=subprocess.run([str(EXE),str(path),encoding,str(leaf),*[str(x) for x in point]],capture_output=True,text=True,timeout=20)
  require(p.returncode==0,label+': '+p.stderr)
  actual=[int(x,16) for x in p.stdout.split()];expected=reference(b,encoding,leaf,point)
  require(actual==expected,label+': float bits differ')
  records.append(dict(label=label,encoding=encoding,leaf=leaf,point=point,channel_bits=[hex(x) for x in actual]))
 for leaf in range(5):
  for encoding in ('hdr','ldr'):
   for point in [(-19,-301,7),(238,112,514),(12.125,-59.75,320.03125)]:check(path,b,encoding,leaf,point,'controlled')
 require(reference(b,'hdr',1,(0,0,0))==[0]*18,'One-hop fixture must stay empty')
 require(reference(b,'hdr',2,(0,0,0))==[0]*18,'Zero-reference fixture must stay empty')
 sample_offset=struct.unpack_from('<I',b,8+55*16)[0]
 for exponent in (-128,-127,-64,-16,-1,0,1,64,120):
  variant=bytearray(b)
  for face in range(6):variant[sample_offset+2*28+face*4+3]=exponent%256
  exponent_path=run/('exponent-'+str(exponent)+'.bsp');exponent_path.write_bytes(variant)
  check(exponent_path,variant,'hdr',4,(12.125,-59.75,320.03125),'signed exponent '+str(exponent))
 for point in [('nan','0','0'),('inf','0','0'),('3e38','0','0')]:
  p=subprocess.run([str(EXE),str(path),'hdr','0',*point],capture_output=True,text=True,timeout=20)
  require(p.returncode!=0,'Invalid query accepted')
 for leaf in ('999','4294967295'):
  p=subprocess.run([str(EXE),str(path),'hdr',leaf,'0','0','0'],capture_output=True,text=True,timeout=20)
  require(p.returncode!=0,'Invalid leaf accepted')
 map_bytes=MAP.read_bytes()
 for leaf,point in [(1609,(-2026.15625,-4541.78125,-351.90625)),(1609,(-2026.15625,-4541.78125,-310.90625)),(1045,(-299.5,-2278.5,-239.9375)),(1609,(-2029.45508,-4545.72363,-316.088257)),(1609,(-2047.16455,-4561.77783,-320.608307)),(1113,(-484.5,-2271.5,-183.9375)),(2018,(-2944,-1384,253.237))]:
  check(MAP,map_bytes,'hdr',leaf,point,'benchmark diagnostic point')
 require(digest(MAP)==before,'Original map changed')
 receipt=dict(status='PASS_RECONSTRUCTED_FLOAT_CONTRACT',executable=str(EXE),sha256=digest(EXE),amd64=True,exact_channel_comparisons=len(records)*18,rejected_queries=5,map_sha256=before,records=records,limitations=['Independent float reference, not live installed-engine execution','Diagnostic points are not recovered model anchors','No renderer integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
 print(json.dumps(dict(status=receipt['status'],cases=len(records),channels=len(records)*18,receipt=str(run/'results.json'))))
if __name__=='__main__':main()
