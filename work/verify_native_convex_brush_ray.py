from pathlib import Path
import hashlib,json,random,struct,subprocess,sys,tempfile
from pe_image import Image
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/convex-brush-oracle-20261015'
ENGINE=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
MAP=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\maps\mtt_soldier.bsp')
DIGEST='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
def require(ok,message):
 if not ok:raise RuntimeError(message)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def cube():
 b=bytearray(1036);struct.pack_into('<II',b,0,0x50534256,20)
 def append(i,v,p):struct.pack_into('<IIII',b,8+16*i,len(b),len(p),v,0);b.extend(p)
 planes=[]
 for axis in range(3):
  for sign in (1,-1):n=[0,0,0];n[axis]=sign;planes.append(struct.pack('<4fi',*n,1,3))
 append(1,0,b''.join(planes));append(5,0,b'');leaf=bytearray(32);struct.pack_into('<H',leaf,26,1);append(10,1,leaf);model=bytearray(48);struct.pack_into('<i',model,36,-1);append(14,0,model);append(17,0,b'\0\0');append(18,0,struct.pack('<iii',0,6,1));append(19,0,b''.join(struct.pack('<Hhhh',i,-1,-1,0) for i in range(6)));append(26,0,b'');return b
def main():
 image=Image(ENGINE);require(hashlib.sha256(image.b).hexdigest()==DIGEST,'Engine changed');fragments=[('nativeBrushBytes',0x10147690,0x1014791f),('nativePlaneCopyBytes',0x100e9720,0x100e975d),('nativeSurfaceBytes',0x10146440,0x1014646a)]
 header='// Generated checked general brush kernel/helpers; test only.\n';blobs=[]
 for name,start,end in fragments:
  raw=image.b[image.offset(start):image.offset(start)+(end-start)];blobs.append(raw);header+='static const unsigned char '+name+'[]={'+','.join('0x%02x'%x for x in raw)+'};\n'
 hp=ROOT/'work/native_convex_brush_ray_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared checked original brush kernel/helpers');return
 require(hp.read_text()==header,'Native header stale');run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));before=sha(MAP);rng=random.Random(20151015);records=[]
 for name,machine in [('brush_ray_native_x86',0x14c),('brush_ray_candidate_x64',0x8664)]:
  path=OUT/(name+'.exe');b=path.read_bytes();pe=struct.unpack_from('<I',b,60)[0];require(struct.unpack_from('<H',b,pe+4)[0]==machine,'Wrong architecture')
  if machine==0x14c:require(all(blob in b for blob in blobs),'Original kernel/helper absent')
  records.append(dict(executable=str(path),sha256=sha(path),machine=hex(machine)))
 controlled=run/'cube.bsp';controlled.write_bytes(cube());queries=[]
 points=[(-2,0,0),(2,0,0),(0,0,0),(1,0,0),(-1,0,0),(0,2,0),(0,0,2),(0.5,0.5,0.5),(2,2,2)]
 states=[(1,0,0,0xffffffff),(0.125,0,0,0),(0.5,0.25,1,0),(1,0.75,3,0)]
 for start in points:
  for end in points:
   for fraction,left,flags,side in states:queries.append((0,*start,*end,fraction,left,flags,0x123456,side))
 for _ in range(4096):
  state=rng.choice(states);queries.append((0,*[rng.uniform(-3,3) for _ in range(6)],state[0],state[1],state[2],0x123456,state[3]))
 raw=MAP.read_bytes()
 def lump(i):return struct.unpack_from('<II',raw,8+16*i)
 bo,bn=lump(18);so,_=lump(19);po,_=lump(1);benchmark=[]
 for index in range(bn//12):
  first,count,contents=struct.unpack_from('<III',raw,bo+12*index);low=[None]*3;high=[None]*3
  for side in range(first,first+count):
   plane=struct.unpack_from('<H',raw,so+8*side)[0];normal=struct.unpack_from('<4f',raw,po+20*plane)
   for axis in range(3):
    if all(normal[j]==(normal[axis] if j==axis else 0) for j in range(3)):
     if normal[axis]==1:high[axis]=normal[3]
     elif normal[axis]==-1:low[axis]=-normal[3]
  center=tuple((low[i]+high[i])/2 if low[i] is not None and high[i] is not None else 0 for i in range(3))
  for state in states:
   start=(center[0]-64,center[1],center[2]);end=(center[0]+64,center[1],center[2]);benchmark.append((index,*start,*end,state[0],state[1],state[2],0x123456,first if count else 0xffffffff))
  benchmark.append((index,*center,center[0]+128,center[1],center[2],1,0,0,0x123456,0xffffffff))
 datasets=[(controlled,queries),(MAP,benchmark)];summary=[]
 for dataset,(map_path,qs) in enumerate(datasets):
  inp=run/(str(dataset)+'-queries.bin');inp.write_bytes(b''.join(struct.pack('<I8f3I',*q) for q in qs));outputs=[]
  for record in records:
   target=run/(str(dataset)+'-'+Path(record['executable']).stem+'.bin');p=subprocess.run([record['executable'],str(map_path),str(inp),str(target)],capture_output=True,text=True,timeout=40);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));outputs.append(target.read_bytes())
  require(len(outputs[0])==len(qs)*36 and len(outputs[1])==len(outputs[0]),'Output size mismatch')
  for i in range(len(qs)):
   a,c=outputs[0][36*i:36*i+36],outputs[1][36*i:36*i+36]
   require(a==c,'Dataset '+str(dataset)+' case '+str(i)+' query '+str(qs[i])+' native '+str(struct.unpack('<9I',a))+' x64 '+str(struct.unpack('<9I',c))+' artifacts '+str(run))
  summary.append(dict(map=str(map_path),cases=len(qs),output_sha256=hashlib.sha256(outputs[0]).hexdigest()))
 require(sha(MAP)==before and sha(ENGINE)==DIGEST,'Original files changed');receipt=dict(status='PASS_NATIVE_GENERAL_BRUSH_KERNEL',datasets=summary,cases=sum(x['cases'] for x in summary),executables=records,engine_sha256=DIGEST,map_sha256=before,limits=['All brushes forced through general encoding; optimized box path not tested','Surface table synthetic zero metadata; original surface helper executed','No displacement collision or full world traversal','No renderer integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=receipt['cases'],receipt=str(run/'results.json'))))
if __name__=='__main__':main()
