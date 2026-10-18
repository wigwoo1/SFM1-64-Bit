from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
print(hex(im.word(0x10b37094+0xc8)))
print(im.string(0x10a8e744))
