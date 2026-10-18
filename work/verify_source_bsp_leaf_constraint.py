"""Original x86 plane clipping versus owned x64, using actual BSP node data."""
from pathlib import Path
import hashlib,json,random,struct,subprocess,sys,tempfile
from pe_image import Image
ROOT=Path(__file__).resolve().parent.parent
ENGINE=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\engine.dll')
MAP=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\maps\mtt_soldier.bsp')
DIGEST='c084b859142f39e3a1040a550e5e2edf44940f1f5fb9fe46cf257f25bb5b5e4f'
OUT=ROOT/'outputs/bsp-leaf-constraint-20261015'
def require(ok,message):
 if not ok:raise RuntimeError(message)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def fixture(root):
 b=bytearray(1036);struct.pack_into('<II',b,0,0x50534256,20)
 def append(i,version,payload):struct.pack_into('<IIII',b,8+16*i,len(b),len(payload),version,0);b.extend(payload)
 append(1,0,b''.join(struct.pack('<4fi',*v) for v in [(1,0,0,0,0),(0,1,0,50,1),(0.6,0.8,0,3,3)]))
 nodes=bytearray(96)
 for i,values in enumerate([(0,1,2),(1,-1,-2),(2,-3,-4)]):struct.pack_into('<Iii',nodes,32*i,*values)
 append(5,0,nodes);append(10,1,bytes(128));model=bytearray(48);struct.pack_into('<i',model,36,root);append(14,0,model);return b
def main():
 image=Image(ENGINE);require(hashlib.sha256(image.b).hexdigest()==DIGEST,'Engine changed')
 code=image.b[image.offset(0x10145690):image.offset(0x10145690)+0x148]
 header='// Generated checked original leaf constraint; test only.\nstatic const unsigned char nativeLeafConstraintBytes[]={'+','.join('0x%02x'%v for v in code)+'};\n'
 hp=ROOT/'work/native_bsp_leaf_constraint_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared checked native leaf constraint');return
 require(hp.read_text()==header,'Stale instruction header')
 OUT.mkdir(parents=True,exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));before=sha(MAP);rng=random.Random(63452026)
 records=[]
 for name,machine in [('leaf_constraint_native_x86',0x14c),('leaf_constraint_candidate_x64',0x8664)]:
  exe=OUT/(name+'.exe');raw=exe.read_bytes();pe=struct.unpack_from('<I',raw,60)[0];require(struct.unpack_from('<H',raw,pe+4)[0]==machine,'Wrong architecture')
  if machine==0x14c:require(code in raw,'Compiled oracle lacks original body')
  records.append(dict(executable=str(exe),sha256=sha(exe),machine=hex(machine)))
 base=[(10,60,0,0.5,-10,-60,0),(-10,0,0,0.5,0,0,0),(0,60,0,0.5,-10,60,0),(10,50,0,0.5,10,-10,0),(-10,0,0,0.5,-10,0,0),(0,50,3,0.5,0,50,3)]
 base.extend(tuple([*[rng.uniform(-512,512) for _ in range(3)],rng.choice([0,0.5,2]),*[rng.uniform(-512,512) for _ in range(3)]]) for _ in range(2048))
 datasets=[]
 for root in (0,1,-1):
  path=run/('fixture-root-'+str(root)+'.bsp');path.write_bytes(fixture(root));datasets.append((path,base))
 benchmark=[]
 for start in [(-2026.15625,-4541.78125,-351.90625),(-2026.15625,-4541.78125,-310.90625),(-299.5,-2278.5,-239.9375)]:
  for _ in range(256):benchmark.append((*start,0.5,*[v+rng.uniform(-128,128) for v in start]))
 benchmark.extend(tuple([rng.uniform(-6000,1000),rng.uniform(-6000,1000),rng.uniform(-1000,1000),0.5,rng.uniform(-6000,1000),rng.uniform(-6000,1000),rng.uniform(-1000,1000)]) for _ in range(2048))
 datasets.append((MAP,benchmark));comparisons=[];total=0
 for index,(map_path,queries) in enumerate(datasets):
  inp=run/('queries-'+str(index)+'.bin');inp.write_bytes(b''.join(struct.pack('<7f',*q) for q in queries));outputs=[]
  for record in records:
   target=run/(str(index)+'-'+Path(record['executable']).stem+'.bin');p=subprocess.run([record['executable'],str(map_path),str(inp),str(target)],capture_output=True,text=True,timeout=30);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));outputs.append(target.read_bytes())
  require(outputs[0]==outputs[1] and len(outputs[0])==len(queries)*12,'Native/x64 clipping mismatch: '+str(index))
  if index==0:
   require(struct.unpack_from('<3f',outputs[0])==(0.5,50.5,0.0),'Controlled axial crossing failed')
   require(struct.unpack_from('<3f',outputs[0],12)==(0.0,0.0,0.0),'Native negative-side endpoint tie differs')
   for i in (4,5):require(outputs[0][i*12:(i+1)*12]==struct.pack('<3f',*base[i][4:]),'Zero-length segment modified')
  if index==2:require(outputs[0]==b''.join(struct.pack('<3f',*q[4:]) for q in queries),'Leaf-only world must preserve targets')
  comparisons.append(dict(map=str(map_path),cases=len(queries),output_sha256=hashlib.sha256(outputs[0]).hexdigest()));total+=len(queries)
 rejected=0
 for query in [(float('nan'),0,0,0.5,0,0,0),(0,0,0,-1,0,0,0),(0,0,0,0.5,0,float('inf'),0)]:
  inp=run/('bad-'+str(rejected)+'.bin');inp.write_bytes(struct.pack('<7f',*query));p=subprocess.run([records[1]['executable'],str(datasets[0][0]),str(inp),str(run/'rejected.bin')],capture_output=True,text=True,timeout=15);require(p.returncode!=0,'Malformed query accepted');rejected+=1
 require(sha(MAP)==before and sha(ENGINE)==DIGEST,'Original files changed')
 result=dict(status='PASS_NATIVE_LEAF_CONSTRAINT',cases=total,exact_float_comparisons=total*3,rejected_queries=rejected,executables=records,datasets=comparisons,original_map_sha256=before,engine_sha256=DIGEST,limits=['Original low-level routine on reconstructed runtime node layout','Actual BSP nodes used; no synthetic clipping dependency','No brush/displacement/prop collision trace','No final model-lighting anchor or renderer integration'])
 (run/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(dict(status=result['status'],cases=total,receipt=str(run/'results.json'))))
if __name__=='__main__':main()
