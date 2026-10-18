from pe_image import Image
im=Image(r'F:\SteamLibrary\steamapps\common\SourceFilmmaker\game\bin\tools\ifm.dll')
print('base element slot4:',hex(im.word(0x10b37350)))
