from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
needle=(0x1000cb10).to_bytes(4,'little');start=0
while True:
 p=im.b.find(needle,start)
 if p<0:break
 start=p+1
 for rva,size,raw in im.sections:
  if raw<=p<raw+size:
   va=im.base+rva+p-raw
   try:
    vt=va-16;col=im.word(vt-4);desc=im.word(col+12);name=im.string(desc+8)
    print(hex(vt),name)
   except Exception:pass
