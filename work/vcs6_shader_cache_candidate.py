"""Checked reader for locally observed VCS6 LZMA shader-cache layout."""
import struct,lzma
class Vcs6ShaderCache:
 def __init__(self,data):
  if len(data)>256*1024*1024:raise ValueError('cache source size limit')
  self.data=bytes(data)
  def word(offset):
   if offset<0 or offset+4>len(self.data):raise ValueError('cache word bounds')
   return struct.unpack_from('<I',self.data,offset)[0]
  self.word=word
  if len(self.data)<40 or word(0)!=6:raise ValueError('VCS6 required')
  self.total_combos=word(4);self.dynamic_combos=word(8);count=word(20)
  if not self.dynamic_combos or not self.total_combos or self.total_combos%self.dynamic_combos or count<2 or count>(len(self.data)-28)//8:raise ValueError('cache combo header')
  self.static_count=self.total_combos//self.dynamic_combos
  self.entries=[(word(28+i*8),word(32+i*8)) for i in range(count)]
  if self.entries[-1]!=(0xffffffff,len(self.data)):raise ValueError('cache index sentinel')
  ids=[key for key,_ in self.entries[:-1]]
  if ids!=sorted(set(ids)) or any(key>=self.static_count for key in ids):raise ValueError('cache static ids')
  end=28+count*8;aliases=word(end);end+=4
  if aliases>(len(self.data)-end)//8:raise ValueError('cache aliases bounds')
  pairs=[(word(end+i*8),word(end+4+i*8)) for i in range(aliases)];end+=aliases*8
  if [key for key,_ in pairs]!=sorted(set(key for key,_ in pairs)) or any(key>=self.static_count or key in ids or target not in ids for key,target in pairs):raise ValueError('cache alias identity')
  self.aliases=dict(pairs)
  offsets=[offset for _,offset in self.entries]
  if offsets[0]!=end or offsets!=sorted(set(offsets)):raise ValueError('cache payload offsets')
  self.payloads={};total_decoded=0
  for i,(key,start) in enumerate(self.entries[:-1]):
   finish=offsets[i+1];position=start;records={};ended=False
   while position<finish:
    token=word(position);position+=4
    if token==0xffffffff:
     if position!=finish:raise ValueError('cache block sentinel trailing data')
     ended=True;break
    size=token&0x3fffffff
    if token>>30!=1 or size<17 or position+size>finish:raise ValueError('unsupported cache block encoding or bounds')
    payload=self.data[position:position+size];position+=size
    if payload[:4]!=b'LZMA':raise ValueError('cache LZMA magic')
    decoded_size,compressed_size=struct.unpack_from('<II',payload,4)
    total_decoded+=decoded_size
    if total_decoded>256*1024*1024:raise ValueError('cache decoded size limit')
    if compressed_size!=size-17 or decoded_size<8 or decoded_size>64*1024*1024:raise ValueError('cache LZMA size')
    prop=payload[12];lc=prop%9;prop//=9;lp=prop%5;pb=prop//5;dictionary=struct.unpack_from('<I',payload,13)[0]
    if pb>4 or dictionary>64*1024*1024:raise ValueError('cache LZMA properties')
    decoder=lzma.LZMADecompressor(format=lzma.FORMAT_RAW,filters=[{'id':lzma.FILTER_LZMA1,'lc':lc,'lp':lp,'pb':pb,'dict_size':dictionary}]);decoded=decoder.decompress(payload[17:],max_length=decoded_size)
    if len(decoded)!=decoded_size or decoder.unused_data:raise ValueError('cache LZMA decoded length')
    # Installed blocks use declared output length, sometimes without EOS.
    # Decoding beyond that length can emit range-coder padding as extra bytes.
    at=0
    while at<len(decoded):
     if at+8>len(decoded):raise ValueError('dynamic record header')
     dynamic,length=struct.unpack_from('<II',decoded,at);at+=8
     if dynamic>=self.dynamic_combos or dynamic in records or length<8 or length%4 or at+length>len(decoded):raise ValueError('dynamic record bounds')
     shader=decoded[at:at+length];at+=length;version=struct.unpack_from('<I',shader)[0]
     if version>>16 not in (0xfffe,0xffff) or shader[-4:]!=b'\xff\xff\x00\x00':raise ValueError('D3D shader envelope')
     records[dynamic]=shader
   if not ended:raise ValueError('missing cache block sentinel')
   self.payloads[key]=records
 def shader(self,static,dynamic):
  if static<0 or static>=self.static_count or dynamic<0 or dynamic>=self.dynamic_combos:raise ValueError('shader combo range')
  return self.payloads[self.aliases.get(static,static)][dynamic]

 def shader_for_selector(self,static_combo_argument,dynamic_index):
  # Installed 0x10036028 divides record combo by cached dynamic count.
  # Nonnegative supported inputs use floor division; negative native IDs
  # and invalid ranges are intentionally excluded by this owned reader.
  if static_combo_argument<0 or static_combo_argument>=self.total_combos:raise ValueError('selector static combo range')
  return self.shader(static_combo_argument//self.dynamic_combos,dynamic_index)
