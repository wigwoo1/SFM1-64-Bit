from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll');sha=hashlib.sha256(im.b).hexdigest();assert sha=='07cee65141f5c1cca85a73b25c675ca01c0068cf6e758e49b07e38424ac75baa'
rows=[];pending=None
for line in Path('work/shaderapidx9-disassembly.txt').read_text().splitlines():
 m=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
 if m:
  if pending:rows.append(pending)
  pending={'address':int(m[1],16),'bytes':bytes.fromhex(m[2]),'instruction':m[3].strip()}
 elif pending and re.fullmatch(r'\s*(?:[0-9A-F]{2}\s*)+',line):pending['bytes']+=bytes.fromhex(line)
if pending:rows.append(pending)
begin,end=0x100df3a0,0x100df41a;selected=[r for r in rows if begin<=r['address']<end];starts={r['address'] for r in selected};cursor=begin;edges=[]
for r in selected:
 a,b=r['address'],r['bytes'];assert a==cursor;cursor+=len(b);assert im.b[im.offset(a):im.offset(a)+len(b)]==b;assert b[0]!=0xe8
 if b[0]==0xe9:target=a+5+struct.unpack('<i',b[1:])[0];assert target==0x100e7322;edges.append({'address':hex(a),'target':hex(target),'external':True})
 elif b[0]==0xeb or 0x70<=b[0]<=0x7f:
  target=a+2+struct.unpack('<b',b[1:2])[0];assert target in starts;edges.append({'address':hex(a),'target':hex(target),'external':False})
assert cursor==end
raw=im.b[im.offset(begin):im.offset(end)];assert len(raw)==122
opt=im.u32(60)+24;rv,size=im.u32(opt+136),im.u32(opt+140);pos=im.offset(im.base+rv);limit=pos+size;fixups=[]
while pos<limit:
 page,length=im.u32(pos),im.u32(pos+4);assert length>=8 and pos+length<=limit
 for at in range(pos+8,pos+length,2):
  entry=im.u16(at);a=im.base+page+(entry&4095)
  if entry>>12==3 and begin<=a<end:assert im.word(a)==0x101b610c;fixups.append(a-begin)
 pos+=length
assert len(fixups)==1
Path('outputs/restore-byte-fill-original.bin').write_bytes(raw)
out={'sha256':sha,'begin':hex(begin),'end':hex(end),'bytes':122,'edges':edges,'flag_fixups':fixups,'flag_cell':'0x101b610c','instructions':[{'address':hex(r['address']),'bytes':r['bytes'].hex(),'instruction':r['instruction']} for r in selected],'contract':'cdecl(destination,fill,count), returns destination; lowbyte fill, zero count avoids writes; scalar alignment/dword/tail stores. Zero fill count>=128 with nonzero flag tailjumps to100e7322.','limits':'Scalar body exact-byte recovery only. External fastpath remains unverified; scalar native execution and identity initializer composition pending.'}
Path('outputs/sfm64-restore-byte-fill-recovery.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'restore_byte_fill_bytes=122 branch_edges={len(edges)} global_fixups=1 external_fastpath_pending')
