from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
needle=(0x10b3737c).to_bytes(4,'little');p=0
while True:
 p=im.b.find(needle,p)
 if p<0:break
 for rva,size,raw in im.sections:
  if raw<=p<raw+size:
   address=im.base+rva+p-raw
   print(hex(address),[hex(im.word(address+4*i)) for i in range(9)])
 p+=1
