from pathlib import Path
import json,re
r=json.loads(Path('outputs/sfm64-owner-reset-cleanup-recovery.json').read_text());rows=[x for x in r['instructions'] if 0x10015bd0<=int(x['address'],16)<0x1001624f];allocations=[]
for i,row in enumerate(rows):
 if row['instruction']!='call        10003990':continue
 previous=rows[max(0,i-18):i];counts=[];strides=[]
 for x in previous:
  m=re.search(r'mov\s+eax,dword ptr \[(?:eax|edx|ecx)\+([0-9A-F]+)h\]',x['instruction'])
  if m and 0x528<=int(m[1],16)<=0x53c:counts.append(int(m[1],16))
  m=re.search(r'mov\s+edx,([0-9A-F]+)h?',x['instruction'])
  if m:strides.append(int(m[1],16))
 assert counts and strides
 matches=[re.fullmatch(r'mov\s+dword ptr \[esi\+([0-9A-F]+)h\],eax',x['instruction']) for x in rows[i+1:i+4]];m=next((x for x in matches if x),None);assert m
 allocations.append({'call':row['address'],'owner_pointer_offset':hex(int(m[1],16)),'capability_count_offset':hex(counts[-1]),'element_bytes':strides[-1],'size_policy':'unsigned32 multiply; overflow requestsffffffff'})
assert len(allocations)==12
out={'source_sha256':r['sha256'],'allocations_in_order':allocations,'limits':'Pointer fields must widen on x64; contents keep element widths. Allocation/deallocation callback identities and whole helper runtime validation pending.'}
Path('outputs/sfm64-constant-storage-allocation-map.json').write_text(json.dumps(out,indent=2)+'\n')
print(json.dumps(allocations))


