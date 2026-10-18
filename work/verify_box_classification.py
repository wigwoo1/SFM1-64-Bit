import itertools,json,random,struct,subprocess,sys,tempfile
from pathlib import Path
from pe_image import Image
from verify_native_convex_brush_ray import ROOT,ENGINE,MAP,DIGEST,sha,require
def main():
 require(sha(ENGINE)==DIGEST,'Engine changed');image=Image(ENGINE);start=image.offset(0x1014b5a5);raw=image.b[start:start+0xc3];header='// Checked original classification block; test only.\nstatic const unsigned char nativeClassificationBytes[]={'+','.join('0x%02x'%v for v in raw)+'};\n';hp=ROOT/'work/native_box_classification_bytes.h'
 if '--prepare' in sys.argv:hp.write_text(header);print('Prepared classification block');return
 require(hp.read_text()==header,'Stale block');out=ROOT/'outputs/box-classification-20261015';run=Path(tempfile.mkdtemp(prefix='verification-',dir=out));qs=[]
 for types in itertools.product((0,1,2,3,255),repeat=6):qs.append(struct.pack('<H6B',6,*types))
 for count in (0,1,2,3,4,5,7,8,65534,65535):
  for types in ((0,)*6,(255,)*6,(0,1,2,0,1,2)):qs.append(struct.pack('<H6B',count,*types))
 before=sha(MAP);b=MAP.read_bytes();po=struct.unpack_from('<I',b,24)[0];so=struct.unpack_from('<I',b,8+19*16)[0];bo,bn=struct.unpack_from('<II',b,8+18*16)
 for i in range(bn//12):
  first,count,_=struct.unpack_from('<iii',b,bo+12*i);types=[0]*6
  if count==6:
   for j in range(6):p=struct.unpack_from('<H',b,so+8*(first+j))[0];types[j]=b[po+20*p+16]
  qs.append(struct.pack('<H6B',count,*types))
 inp=run/'queries.bin';inp.write_bytes(b''.join(qs));results=[];executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=out/('classification_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong architecture')
  if name=='native_x86':require(raw in binary,'Original instructions absent')
  target=run/(name+'.bin');p=subprocess.run([str(exe),str(inp),str(target)],capture_output=True,text=True,timeout=40);target.with_suffix('.log').write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));results.append(target.read_bytes());executables.append(dict(executable=str(exe),sha256=sha(exe)))
 require(len(results[0])==len(results[1])==len(qs)*2,'Output length');different=[i for i in range(len(qs)) if results[0][2*i:2*i+2]!=results[1][2*i:2*i+2]];receipt=dict(status='PASS' if not different else 'FAIL',cases=len(qs),benchmark_cases=bn//12,different_cases=different[:20],executables=executables,engine_sha256=sha(ENGINE),map_sha256=sha(MAP),limits=['Original classification block with synthetic loader frame and exit RET','Only selection/count mutation observed, not allocation or full loader','65535 input sentinel indistinguishable from unchanged count in output','No editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],cases=len(qs),receipt=str(run/'results.json'))));require(not different,'Classification differs');require(sha(ENGINE)==DIGEST and sha(MAP)==before,'Original changed')
if __name__=='__main__':main()
