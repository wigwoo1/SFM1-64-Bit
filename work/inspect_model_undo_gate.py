from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
print('record gate',hex(im.word(0x10b37094+0xf8)))
print('submit',hex(im.word(0x10b37094+0x13c)))
