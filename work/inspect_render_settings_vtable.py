from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
def va(p):
 for rva,size,raw in im.sections:
  if raw<=p<raw+size:return im.base+rva+p-raw
 raise ValueError()
def refs(v):
 n=v.to_bytes(4,'little');start=0
 while True:
  p=im.b.find(n,start)
  if p<0:return
  start=p+1
  try:yield va(p)
  except ValueError:pass
for name in [b'.?AVCDmeRenderSettings@@',b'.?AVCDmeSettings@@',b'.?AVCDmeSFMSettings@@']:
 p=im.b.find(name+b'\0')
 if p<0:continue
 desc=va(p)-8
 for ref in refs(desc):
  col=ref-12
  try:
   if im.word(col)!=0:continue
   for pointer in refs(col):
    vt=pointer+4
    slot=im.word(vt+16)
    if 0x10001000<=slot<0x10a20000:print(name.decode(),hex(vt),hex(slot))
  except ValueError:pass
