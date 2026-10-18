"""Compare owned x64 line-box results against checked original x86 instructions."""
from pathlib import Path
import hashlib,json,struct,subprocess,tempfile
from verify_native_optimized_box import ROOT,ENGINE,DIGEST,require,sha

def main():
 require(sha(ENGINE)==DIGEST,'Original engine changed')
 out=ROOT/'outputs/optimized-box-x64-20261015'
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=out))
 source=ROOT/'outputs/convex-brush-oracle-20261015/verification-4vmweidj'
 queries=source/'0-queries.bin'; records=[]; results=[]
 for name,exe,machine in [('native',ROOT/'outputs/optimized-box-native-20261015/optimized_box_native_x86.exe',0x14c),('owned',out/'optimized_box_candidate_x64.exe',0x8664)]:
  binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0]
  require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Wrong executable architecture')
  target=run/(name+'.bin')
  process=subprocess.run([str(exe),str(source/'cube.bsp'),str(queries),str(target)],capture_output=True,text=True,timeout=30)
  (run/(name+'.log')).write_text(process.stdout+process.stderr)
  require(process.returncode==0,process.stdout+process.stderr)
  results.append(target.read_bytes());records.append(dict(executable=str(exe),sha256=sha(exe),machine=hex(machine)))
 require(len(results[0])==len(results[1])==4420*36,'Wrong result count')
 differences=[]
 for i in range(4420):
  a=struct.unpack_from('<9I',results[0],36*i);b=struct.unpack_from('<9I',results[1],36*i)
  if a!=b:differences.append(dict(case=i,native=a,owned=b,query=struct.unpack_from('<12I',queries.read_bytes(),48*i)))
 receipt=dict(status='PASS' if not differences else 'FAIL',cases=4420,exact_words=4420*9,different_cases=len(differences),examples=differences[:20],executables=records,engine_sha256=sha(ENGINE),limits=['Controlled six-plane unit box and zero-extent line rays only','Synthetic surface metadata and native normal globals','No map-loader box classification, displacement collision or editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n')
 print(json.dumps(dict(status=receipt['status'],different_cases=len(differences),receipt=str(run/'results.json'))))
 require(not differences,'Original and owned box results differ')
 require(sha(ENGINE)==DIGEST,'Original engine changed')
if __name__=='__main__':main()
