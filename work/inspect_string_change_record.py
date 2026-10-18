from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
vt=0x10b3820c;col=im.word(vt-4);desc=im.word(col+12)
print(im.string(desc+8));print(im.string(0x10b38234))
