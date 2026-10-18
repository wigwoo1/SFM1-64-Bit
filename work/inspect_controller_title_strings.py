from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for va in (0x10ae7604,0x10ae756c):print(hex(va),repr(im.string(va)))
