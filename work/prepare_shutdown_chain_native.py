from pathlib import Path
import json,struct
exec(Path('work/recover_command_buffer_creation.py').read_text().split('vertex=region')[0])
r=json.loads(Path('outputs/sfm64-device-manager-cleanup-services.json').read_text());assert r['sha256']==sha
bodies=[region(0x100290e0,0x10029101),region(0x10027c50,0x10027c8b),region(0x100ddfd0,0x100ddff7),region(0x100d6120,0x100d6169),region(0x100dbe20,0x100dbe25),region(0x100d4db0,0x100d4ded),region(0x100d9760,0x100d977e),r['owned_recursive_cleanup'],r['free_thunk']]
allowed_calls={int(body['start'],16) for body in bodies}
opt=im.u32(60)+24;relocations=[];rva,size=struct.unpack_from('<II',im.b,opt+136);cursor=im.offset(im.base+rva);end=cursor+size
while cursor<end:
 page,length=struct.unpack_from('<II',im.b,cursor);assert length>=8 and cursor+length<=end
 for o in range(cursor+8,cursor+length,2):
  entry=im.u16(o);assert entry>>12 in (0,3)
  if entry>>12==3:relocations.append(im.base+page+(entry&4095))
 cursor+=length
assert cursor==end
header='#pragma once\n';metadata=[]
for i,body in enumerate(bodies):
 begin=int(body['start'],16);limit=int(body['end'],16);raw=im.b[im.offset(begin):im.offset(begin)+limit-begin];assert raw==b''.join(bytes.fromhex(x['bytes']) for x in body['instructions'])
 boundaries={int(x['address'],16) for x in body['instructions']};calls=[]
 for x in body['instructions']:
  a=int(x['address'],16);b=bytes.fromhex(x['bytes'])
  if b[0] in (0xe8,0xe9):
   target=a+5+struct.unpack_from('<i',b,1)[0];assert target in allowed_calls;calls.append((a-begin,target))
  elif b[0]==0xeb or 0x70<=b[0]<=0x7f:assert a+2+struct.unpack_from('<b',b,1)[0] in boundaries
 fixups=[(a-begin,im.word(a)) for a in relocations if begin<=a<limit]
 assert all(v in {0x100fa274,0x100fa238,0x10183b54,0x10183b58,0x10190904,0x101b11fc,0x101b1200,0x101908e4,0x1019fcf4,0x1017b7f0,0x101909c4,0x101909c8,0x10190718,0x1019071c} for o,v in fixups)
 header+='static const unsigned char shutdownChainBytes'+str(i)+'[]={'+','.join(map(str,raw))+'};\n'
 for name,entries in [('Fixups',fixups),('Calls',calls)]:header+='static const unsigned shutdownChain'+name+str(i)+'[][2]={'+','.join('{'+str(o)+','+str(v)+'}' for o,v in (entries or [(0,0)]))+'};\n'
 metadata.append(dict(original_start=hex(begin),private_offset=512*i,file_offset=im.offset(begin),size=len(raw),fixups=fixups,calls=calls))
header+='struct ShutdownChainBody{const unsigned char* bytes;unsigned size,fileOffset;const unsigned (*fixups)[2];unsigned fixupCount;const unsigned (*calls)[2];unsigned callCount;};\n'
header+='static const ShutdownChainBody shutdownChainBodies[]={'+','.join('{shutdownChainBytes'+str(i)+','+str(m['size'])+','+str(m['file_offset'])+',shutdownChainFixups'+str(i)+','+str(len(m['fixups']))+',shutdownChainCalls'+str(i)+','+str(len(m['calls']))+'}' for i,m in enumerate(metadata))+'};\n'
assert len(metadata)==9 and sum(m['size'] for m in metadata)==481
assert sum(len(m['fixups']) for m in metadata)==21
assert sum(len(m['calls']) for m in metadata)==11
assert max(m['private_offset']+m['size'] for m in metadata)<=8192
header+='static const unsigned shutdownChainAddressMap[][2]={'+','.join('{'+m['original_start']+','+str(m['private_offset'])+'}' for m in metadata)+'};\n'
Path('work/shutdown_chain_native_init.h').write_text(header)
Path('outputs/sfm64-shutdown-chain-native-preparation.json').write_text(json.dumps(dict(sha256=sha,bodies=metadata,ownership='All direct calls/tail jumps restricted to nine verified original bodies. External virtual providers/KeyValuesSystem/g_pMemAlloc controlled. Native receiver+2c tree/+30 D3D; native node+4/+8 payloads,+14 peer,+18 child.',native_executed=False),indent=2)+'\n')
print('verified_shutdown_chain_graph_bytes='+str(sum(m['size'] for m in metadata)))



