from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
for i in range(118,129):print(i,hex(im.word(0x10b37094+4*i)))
