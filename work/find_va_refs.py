"""Locate literal x86 VA references; candidates need instruction validation."""
import pathlib,struct,sys
b=pathlib.Path(sys.argv[1]).read_bytes(); p=struct.unpack_from('<I',b,60)[0]; opt=p+24
base=struct.unpack_from('<I',b,opt+28)[0]
for i in range(struct.unpack_from('<H',b,p+6)[0]):
 o=opt+struct.unpack_from('<H',b,p+20)[0]+40*i
 size,raw=struct.unpack_from('<II',b,o+16); rva=struct.unpack_from('<I',b,o+12)[0]
 name=b[o:o+8].rstrip(b'\0').decode()
 for arg in sys.argv[2:]:
  needle=struct.pack('<I',int(arg,0)); start=raw
  while True:
   found=b.find(needle,start,raw+size)
   if found<0: break
   print(arg,name,hex(base+rva+found-raw)); start=found+1
