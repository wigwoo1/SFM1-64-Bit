import json, random, struct, subprocess, tempfile
from pathlib import Path
from verify_native_convex_brush_ray import ROOT, ENGINE, DIGEST, sha, require
OUT=ROOT/'outputs/controlled-world-geometry-20261015'
def fixture_world():
 b=bytearray(1036);struct.pack_into('<II',b,0,0x50534256,20)
 def append(i,v,p):struct.pack_into('<IIII',b,8+16*i,len(b),len(p),v,0);b.extend(p)
 append(1,0,struct.pack('<4fi',0,0,1,0,2));node=bytearray(32);struct.pack_into('<Iii',node,0,0,-1,-2);append(5,0,node);append(10,1,bytes(64));model=bytearray(48);struct.pack_into('<i',model,36,0);append(14,0,model);append(17,0,b'');append(18,0,b'');append(19,0,b'');append(26,0,b'');return b
def fixture_mesh(power,folded,contents,flags):
 cells=1<<power;width=cells+1;corners=[(-64.,-64.,0.),(-64.,64.,0.),(64.,64.,0.),(64.,-64.,0.)];positions=[]
 for y in range(width):
  for x in range(width):
   px=-64.+128.*x/cells
   if folded and x==cells//2:px=192.
   positions.append((px,-64.+128.*y/cells,0.))
 alphas=[0. if i%3==0 else 255. for i in range(len(positions))];triangles=[]
 for row in range(cells):
  for column in range(cells):
   index=row*width+column
   for t in ([(index,index+width,index+1),(index+1,index+width,index+width+1)] if index&1 else [(index,index+width,index+width+1),(index,index+width+1,index+1)]):triangles.append((*t,len(triangles)%32))
 return struct.pack('<3I12f',power,contents,flags,*[x for v in corners for x in v])+b''.join(struct.pack('<3f',*p) for p in positions)+struct.pack('<%df'%len(alphas),*alphas)+b''.join(struct.pack('<4H',*t) for t in triangles)
def main():
 require(sha(ENGINE)==DIGEST,'Original engine changed');OUT.mkdir(exist_ok=True);run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));world=run/'controlled.bsp';world.write_bytes(fixture_world());rng=random.Random(2026101593);rows=[];kinds=[]
 def append(point,delta,mask,kind,positions=True):rows.append(struct.pack('<6f3I',*point,*delta,mask,int(positions),1));kinds.append(kind)
 for x in [-65.,-64.,-32.,0.,32.,63.5,64.,80.,128.,191.5,192.,193.]:
  for y in [-64.,-31.5,0.,16.,63.5,64.]:
   for z in [-16.,-.5,0.,.5,16.]:
    for mask in [0,1,128,0x4000,0x4081]:append((x,y,z),(0.,0.,0.),mask,'stationary',len(rows)%2==0)
 for i in range(1024):append((rng.uniform(-66,194),rng.uniform(-66,66),rng.uniform(-16,16)),(0.,0.,0.),rng.choice([1,0x4081]),'stationary',i%2==0)
 for x in [-32.,32.,80.,128.]:
  for z in [-16.,16.]:append((x,16.,z),(0.,0.,-2*z),0x4081,'swept')
 inp=run/'queries.bin';inp.write_bytes(b''.join(rows));records=[];totals=dict(cases=0,stationary_markers=0,stationary_solid_markers=0,swept_markers=0);executables=[]
 for name,machine in [('native_x86',0x14c),('owned_x64',0x8664)]:
  exe=OUT/('geometry_'+name+'.exe');binary=exe.read_bytes();pe=struct.unpack_from('<I',binary,60)[0];require(struct.unpack_from('<H',binary,pe+4)[0]==machine,'Architecture');executables.append(dict(executable=str(exe),sha256=sha(exe)))
 for power in [1,2,3,4]:
  for folded,contents,flags,label in [(False,1,0,'flat'),(True,1,0,'folded'),(True,128,0,'folded-window'),(True,0x4000,0,'folded-grate'),(True,1,8,'disabled')]:
   prefix='p%d-%s'%(power,label);mesh=run/(prefix+'-mesh.bin');mesh.write_bytes(fixture_mesh(power,folded,contents,flags));outputs=[]
   for name in ['native_x86','owned_x64']:
    exe=OUT/('geometry_'+name+'.exe');target=run/(prefix+'-'+name+'.bin');args=[str(exe),str(world),str(mesh),str(inp),str(target)]
    if name=='native_x86':args.append(str(ENGINE))
    p=subprocess.run(args,capture_output=True,text=True,timeout=45);(run/(prefix+'-'+name+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr+' exit='+str(p.returncode));actual=target.read_bytes();require(len(actual)==len(rows)*68,'Output length');outputs.append(actual)
   different=[i for i in range(len(rows)) if outputs[0][68*i:68*i+68]!=outputs[1][68*i:68*i+68]];coverage=dict(stationary_markers=0,stationary_solid_markers=0,swept_markers=0,clear_marker_retained=0)
   witness=False
   for i,kind in enumerate(kinds):
    r=struct.unpack_from('<17I',outputs[0],68*i)
    q=struct.unpack('<6f3I',rows[i])
    if q[6]==0:require(r[10]==0,'Masked geometry reported a hit')
    if power==2 and label=='folded' and q[:3]==(80.,16.,-16.) and kind=='stationary' and q[6]==1:
     # At x=80 the reversed interior strip faces the upward probe. The
     # stationary finish clears solid/contents/fraction but keeps hit metadata.
     normal=struct.unpack_from('<4f',outputs[0],68*i+20);require(r[0]==0x3f800000 and r[1:5]==(0,0,0,0) and r[10]==1 and abs(normal[0])==0 and abs(normal[1])==0 and normal[2]<-.99 and normal[3]==0,'Analytical folded-strip witness');witness=True
    if r[10]:
     coverage['stationary_markers' if kind=='stationary' else 'swept_markers']+=1
     if kind=='stationary':coverage['stationary_solid_markers']+=bool(r[2]);coverage['clear_marker_retained']+=not r[2]
   if power==2 and label=='folded':require(witness,'Analytical witness missing')
   rec=dict(fixture=prefix,contents=contents,flags=flags,cases=len(rows),different_count=len(different),coverage=coverage,analytical_witness=witness);records.append(rec);totals['cases']+=len(rows)
   for key in ['stationary_markers','stationary_solid_markers','swept_markers']:totals[key]+=coverage[key]
   if different:rec['first_difference']=dict(case=different[0],query=struct.unpack('<6f3I',rows[different[0]]),native=struct.unpack_from('<17I',outputs[0],68*different[0]),owned=struct.unpack_from('<17I',outputs[1],68*different[0]))
   (run/'results.json').write_text(json.dumps(dict(status='INCOMPLETE' if not different else 'FAIL',totals=totals,fixtures=records,executables=executables),indent=2)+'\n');require(not different,'Controlled world differs: '+json.dumps(rec))
 require(totals['stationary_markers']>100,'Stationary geometric hit coverage missing');require(all(r['coverage']['stationary_markers']==0 and r['coverage']['swept_markers']==0 for r in records if r['fixture'].endswith('disabled')),'Disabled displacement hit');require(all(r['coverage']['stationary_markers']==0 for r in records if r['fixture'].endswith('-flat')),'Flat one-sided stationary probe hit');require(sha(ENGINE)==DIGEST,'Original engine modified');receipt=dict(status='PASS',totals=totals,fixtures=records,executables=executables,engine_sha256=DIGEST,world_sha256=sha(world),query_sha256=sha(inp),limits=['Controlled prepared runtime geometry; not complete BSP-to-displacement loader validation','Folded interior grid positions deliberately exercise one-sided hits along the base normal; original quad corners retained','Complete original world wrapper and geometric collision routines execute privately with controlled work-pool/memset callbacks','Stationary solid-marker branch remains separate controlled-policy evidence unless covered here','Zero extents only; material metadata, hulls, live engine work pool and editor integration excluded'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status='PASS',totals=totals,receipt=str(run/'results.json'))))
if __name__=='__main__':main()
