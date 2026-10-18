from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
vt=0x10b37a14;col=im.word(vt-4);desc=im.word(col+12)
print(im.string(desc+8));print('initializer',hex(im.word(vt+8)))
