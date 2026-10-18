from pathlib import Path
import collections,hashlib,json,struct,subprocess,tempfile
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'outputs/bsp-world-collision-data-20261015'
EXE=OUT/'world_collision_data_x64.exe'
MAP=Path(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\tf_movies\maps\mtt_soldier.bsp')
def require(ok,message):
 if not ok:raise RuntimeError(message)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def fixture():
 b=bytearray(1036);struct.pack_into('<II',b,0,0x50534256,20)
 def append(i,version,payload):struct.pack_into('<IIII',b,8+16*i,len(b),len(payload),version,0);b.extend(payload)
 append(1,0,struct.pack('<4fi',1,0,0,1,0));append(5,0,b'')
 leaves=bytearray(64);struct.pack_into('<HH',leaves,24,0,2);struct.pack_into('<HH',leaves,56,2,3);append(10,1,leaves)
 model=bytearray(48);struct.pack_into('<i',model,36,-1);append(14,0,model)
 append(17,0,struct.pack('<5H',0,0,1,0,1));append(18,0,struct.pack('<6i',0,1,1,1,1,128));append(19,0,struct.pack('<HhhhHhhh',0,-1,-1,0,0,-1,-1,1));append(26,0,b'');return b
def expected(b):
 def lump(i):return struct.unpack_from('<II',b,8+16*i)
 po,pn=lump(1);no,nn=lump(5);lo,ln=lump(10);mo,_=lump(14);ro,rn=lump(17);bo,bn=lump(18);so,sn=lump(19);_,dn=lump(26)
 pending=[struct.unpack_from('<i',b,mo+36)[0]];seen=set();selected=set()
 while pending:
  node=pending.pop()
  if node>=0:
   if node in seen:continue
   seen.add(node);pending.extend(struct.unpack_from('<ii',b,no+node*32+4))
  else:
   first,count=struct.unpack_from('<HH',b,lo+(-1-node)*32+24)
   selected.update(struct.unpack_from('<H',b,ro+(first+i)*2)[0] for i in range(count))
 words=[pn//20,sn//8,bn//12,len(selected),dn//176,len(seen)];contents=collections.Counter();raw_dispinfo=collections.Counter()
 for index in sorted(selected):
  first,count,mask=struct.unpack_from('<III',b,bo+index*12);words.extend([index,mask,count]);contents[str(mask)]+=1
  for i in range(count):
   plane,texture,disp,bevel=struct.unpack_from('<Hhhh',b,so+(first+i)*8);words.extend([plane,texture&0xffffffff,disp&0xffffffff,bevel&0xffffffff,*struct.unpack_from('<4I',b,po+plane*20)]);raw_dispinfo[str(disp)]+=1
 words.append(ln//32);references=0;duplicate_references=0
 for leaf in range(ln//32):
  first,count=struct.unpack_from('<HH',b,lo+32*leaf+24);ordered=[struct.unpack_from('<H',b,ro+2*(first+j))[0] for j in range(count)];words.extend([struct.unpack_from('<I',b,lo+32*leaf)[0],count,*ordered]);references+=count;duplicate_references+=count-len(set(ordered))
 return struct.pack('<'+'I'*len(words),*words),dict(planes=pn//20,sides=sn//8,all_brushes=bn//12,world_brushes=len(selected),world_nodes=len(seen),displacement_records=dn//176,world_raw_dispinfo=dict(raw_dispinfo),world_contents=dict(contents),leaves=ln//32,ordered_leaf_references=references,within_leaf_duplicate_references=duplicate_references)
def main():
 run=Path(tempfile.mkdtemp(prefix='verification-',dir=OUT));before=sha(MAP);b=fixture();controlled=run/'controlled.bsp';controlled.write_bytes(b);datasets=[]
 exe=EXE.read_bytes();pe=struct.unpack_from('<I',exe,60)[0];require(struct.unpack_from('<H',exe,pe+4)[0]==0x8664,'Not AMD64')
 for i,path in enumerate((controlled,MAP)):
  target=run/(str(i)+'.bin');p=subprocess.run([str(EXE),str(path),str(target)],capture_output=True,text=True,timeout=30);(run/(str(i)+'.log')).write_text(p.stdout+p.stderr);require(p.returncode==0,p.stdout+p.stderr);want,inventory=expected(path.read_bytes());require(target.read_bytes()==want,'Raw collision record mismatch');datasets.append(dict(path=str(path),inventory=inventory,record_sha256=sha(target)))
 require(datasets[0]['inventory']['world_brushes']==1 and datasets[0]['inventory']['all_brushes']==2,'Controlled inline exclusion/dedup fixture invalid')
 def offset(data,i):return struct.unpack_from('<I',data,8+16*i)[0]
 invalid=[]
 for name,change in [
  ('negative-brush',lambda x:struct.pack_into('<i',x,offset(x,18),-1)),
  ('brush-span',lambda x:struct.pack_into('<i',x,offset(x,18)+4,99)),
  ('side-plane',lambda x:struct.pack_into('<H',x,offset(x,19),99)),
  ('leaf-reference',lambda x:struct.pack_into('<H',x,offset(x,17),99)),
  ('leaf-span',lambda x:struct.pack_into('<H',x,offset(x,10)+26,99)),
  ('outside-file',lambda x:struct.pack_into('<I',x,8+18*16,0xfffffff0)),
  ('bevel',lambda x:struct.pack_into('<h',x,offset(x,19)+6,2)),
  ('compression',lambda x:struct.pack_into('<I',x,8+18*16+12,1)),
  ('stride',lambda x:struct.pack_into('<I',x,8+19*16+4,15)),
  ('zero-plane',lambda x:struct.pack_into('<4fi',x,offset(x,1),0,0,0,1,3)),
 ]:
  bad=bytearray(b);change(bad);path=run/(name+'.bsp');path.write_bytes(bad);p=subprocess.run([str(EXE),str(path),str(run/'invalid-output.bin')],capture_output=True,text=True,timeout=15);require(p.returncode!=0,'Invalid input accepted: '+name);invalid.append(name)
 require(sha(MAP)==before,'Original map changed')
 receipt=dict(status='PASS_CHECKED_COLLISION_RECORDS',executable=str(EXE),sha256=sha(EXE),architecture='AMD64',datasets=datasets,rejected_inputs=invalid,original_map_sha256=before,limits=['Collision records only, not ray tracing','Displacements inventoried, not collision-decoded','No editor integration'])
 (run/'results.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps(dict(status=receipt['status'],receipt=str(run/'results.json'),benchmark=datasets[1]['inventory'])))
if __name__=='__main__':main()
