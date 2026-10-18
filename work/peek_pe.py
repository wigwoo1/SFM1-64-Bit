import pathlib,struct,sys
b=pathlib.Path(sys.argv[1]).read_bytes();p=struct.unpack_from('<I',b,60)[0];opt=p+24
base=struct.unpack_from('<I',b,opt+28)[0]
for arg in sys.argv[2:]:
 va=int(arg,0)
 for i in range(struct.unpack_from('<H',b,p+6)[0]):
  o=opt+struct.unpack_from('<H',b,p+20)[0]+40*i
  rva=struct.unpack_from('<I',b,o+12)[0];size,raw=struct.unpack_from('<II',b,o+16)
  if rva<=va-base<rva+size:
   start=raw+va-base-rva
   for j in range(0,96,4): print(f'{va+j:08X} {struct.unpack_from("<I",b,start+j)[0]:08X}')
