from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for vt in (0x10aa9978,0x10aa8804):
 col=im.word(vt-4);print(hex(vt),im.string(im.word(col+12)+8),hex(im.word(vt)),hex(im.word(col+4)))
