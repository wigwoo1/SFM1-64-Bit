"""Extract addressable strings for static disassembly cross references."""
import pathlib, re, struct, sys
p=pathlib.Path(sys.argv[1]); b=p.read_bytes()
u16=lambda o:struct.unpack_from('<H',b,o)[0]
u32=lambda o:struct.unpack_from('<I',b,o)[0]
pe=u32(60); opt=pe+24
base=struct.unpack_from('<Q',b,opt+24)[0] if u16(opt)==0x20b else u32(opt+28)
sections=[]
for i in range(u16(pe+6)):
    o=opt+u16(pe+20)+40*i
    sections.append((u32(o+20),u32(o+16),u32(o+12)))
for m in re.finditer(rb'[ -~]{4,}\x00',b):
    for raw,size,rva in sections:
        if raw<=m.start()<raw+size:
            print(f'{base+rva+m.start()-raw:08X}\t{m.group()[:-1].decode("ascii")}')
            break
