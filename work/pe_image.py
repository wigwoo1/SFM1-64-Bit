"""Minimal checked PE32 address reader for offline recovery."""
import pathlib,struct
class Image:
 def __init__(self,path):
  self.b=pathlib.Path(path).read_bytes();p=self.u32(60);opt=p+24
  if self.b[p:p+4]!=b'PE\0\0' or self.u16(opt)!=0x10b: raise ValueError('PE32 required')
  self.base=self.u32(opt+28);self.sections=[]
  for i in range(self.u16(p+6)):
   o=opt+self.u16(p+20)+40*i
   self.sections.append((self.u32(o+12),self.u32(o+16),self.u32(o+20)))
 def u16(self,o):return struct.unpack_from('<H',self.b,o)[0]
 def u32(self,o):return struct.unpack_from('<I',self.b,o)[0]
 def offset(self,va):
  for rva,size,raw in self.sections:
   if rva<=va-self.base<rva+size:return raw+va-self.base-rva
  raise ValueError('Address lacks file backing: '+hex(va))
 def word(self,va):return self.u32(self.offset(va))
 def string(self,va):
  o=self.offset(va);return self.b[o:self.b.index(b'\0',o)].decode('ascii')
