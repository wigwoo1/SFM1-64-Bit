from pathlib import Path
import hashlib,json,re,struct
from pe_image import Image

path=Path(r'D:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\shaderapidx9.dll')
im=Image(path)
targets={'ring':0x1017e8b8,'produced':0x1017f188,'consumed':0x1017f290,
         'producer_index':0x1017f18c,'consumer_index_candidate':0x1017f294,
         'shifted_ring_900':0x1017f1b8,'pool_table':0x1017e040}
lines=Path('work/shaderapidx9-disassembly.txt').read_text().splitlines()
references=[]
for line in lines:
    match=re.match(r'\s*([0-9A-F]{8}):\s+((?:[0-9A-F]{2}\s+)+)(.*)',line)
    if not match:continue
    address=int(match[1],16);raw=bytes.fromhex(match[2]);instruction=match[3].strip()
    hits=[name for name,value in targets.items() if f'{value:X}h' in instruction]
    if not hits:continue
    offset=im.offset(address)
    assert im.b[offset:offset+len(raw)]==raw
    references.append({'address':hex(address),'targets':hits,'instruction':instruction,'verified_instruction_prefix_bytes':len(raw)})
# Raw literal matches include data and instruction operands, not inferred xrefs.
literals={}
for name,value in targets.items():
    needle=struct.pack('<I',value);offset=0;matches=[]
    while True:
        offset=im.b.find(needle,offset)
        if offset<0:break
        matches.append(offset);offset+=1
    literals[name]={'target':hex(value),'file_offsets':matches}
report={'dll_sha256':hashlib.sha256(im.b).hexdigest(),'verified_disassembly_references':references,
        'raw_literal_matches':literals,'consumer_identified':False,
        'limits':'Bounded direct absolute-address audit only. Relative accesses, shifted bases, indirect pointers, other modules and unrecognized code can hide a consumer. No claim that queue consumption is absent or unreachable.'}
# Inspect loader relocation entries, including pointers in data not rendered
# as instructions. HIGHLOW relocations identify original PE32 absolute slots.
pe=im.u32(60);optional=pe+24
reloc_rva=im.u32(optional+96+5*8);reloc_size=im.u32(optional+96+5*8+4)
relocation_hits=[];highlow_count=0
if reloc_size:
    cursor=im.offset(im.base+reloc_rva);end=cursor+reloc_size
    assert end<=len(im.b)
    while cursor<end:
        assert cursor+8<=end
        page,size=struct.unpack_from('<II',im.b,cursor)
        assert size>=8 and size%2==0 and cursor+size<=end
        for entry_offset in range(cursor+8,cursor+size,2):
            entry=im.u16(entry_offset)
            if entry>>12!=3:continue
            highlow_count+=1;slot=im.base+page+(entry&4095);value=im.word(slot)
            if 0x1017e8b8<=value<0x1017f298:
                relocation_hits.append({'slot':hex(slot),'target':hex(value),'ring_offset':hex(value-0x1017e8b8)})
        cursor+=size
report['highlow_relocation_count']=highlow_count
report['relocated_ring_interior_pointers']=relocation_hits
report['limits']='Direct disassembly references and all HIGHLOW relocation slots in this PE audited. Runtime-computed pointers, relative accesses, shifted bases and other modules remain unresolved; no claim that consumption is absent or unreachable.'
Path('outputs/sfm64-device-queue-consumer-reference-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print('verified_direct_reference_instructions='+str(len(references)))
print('consumer_identified=0 relative_and_indirect_paths_unresolved=1')
print('relocated_ring_interior_pointers='+str(len(relocation_hits))+' highlow_slots='+str(highlow_count))
