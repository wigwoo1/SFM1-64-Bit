import runpy,json,struct
from pathlib import Path
base=runpy.run_path('work/recover_restore_byte_fill.py');im=base['im'];sha=base['sha'];rows=base['rows']
begin,end=0x100e7322,0x100e73dc;selected=[r for r in rows if begin<=r['address']<end];starts={r['address'] for r in selected};cursor=begin;edges=[]
for r in selected:
 a,b=r['address'],r['bytes'];assert a==cursor;cursor+=len(b);assert im.b[im.offset(a):im.offset(a)+len(b)]==b;assert b[0]!=0xe8
 if b[0]==0xe9:target=a+5+struct.unpack('<i',b[1:])[0];assert target in starts;edges.append((a,target))
 elif b[0]==0xeb or 0x70<=b[0]<=0x7f:target=a+2+struct.unpack('<b',b[1:2])[0];assert target in starts;edges.append((a,target))
assert cursor==end
opt=im.u32(60)+24;rv,size=im.u32(opt+136),im.u32(opt+140);pos=im.offset(im.base+rv);limit=pos+size
while pos<limit:
 page,length=im.u32(pos),im.u32(pos+4);assert length>=8 and pos+length<=limit
 for at in range(pos+8,pos+length,2):
  entry=im.u16(at);a=im.base+page+(entry&4095);assert not(entry>>12==3 and begin<=a<end)
 pos+=length
raw=im.b[im.offset(begin):im.offset(end)];assert len(raw)==186
Path('outputs/restore-fill-fastpath-original.bin').write_bytes(raw)
Path('work/restore_fill_fastpath_native_init.h').write_text(f'static const unsigned restoreFillFastOffset={im.offset(begin)},restoreFillFastSize={len(raw)};\n')
out={'sha256':sha,'begin':hex(begin),'end':hex(end),'bytes':186,'branches':edges,'fixups':0,'contract':'Internal tail target expects ECX destination/EDX count>=128; saves original destination, aligns writes to16bytes, SSE2 zero stores, scalar tail. Returns directly to original caller.','limits':'Exact recovery only; execution via original scalar dispatcher pending.'}
Path('outputs/sfm64-restore-fill-fastpath-recovery.json').write_text(json.dumps(out,indent=2)+'\n')
print(f'restore_zero_fastpath_bytes=186 internal_branches={len(edges)} verified')
