from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for va in (0x10aa97d8,0x10aa97d0):print(hex(va),repr(im.string(va)))
