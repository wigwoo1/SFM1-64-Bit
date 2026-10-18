#include "vtf_block_decode.h"
#include <iostream>
int main(){try{
 static_assert(sizeof(void*)==8,"x64 required");
 unsigned decoded=0,rejected=0;
 for(uint32_t minor:{2u,3u,4u}){
  const size_t header=minor==2?80:88;
  std::vector<uint8_t>b(header+2*(4+16));
  auto put=[&](size_t o,uint32_t v){std::memcpy(b.data()+o,&v,4);};
  auto half=[&](size_t o,uint16_t v){std::memcpy(b.data()+o,&v,2);};
  put(0,0x00465456);put(4,7);put(8,minor);put(12,uint32_t(header));half(16,2);half(18,2);half(24,2);half(63,1);put(52,0);b[56]=2;
  if(minor!=2){put(68,1);put(80,0x30);put(84,uint32_t(header));}
  // Independent fixture: small mip frame red then green; large mip blue then white.
  const uint8_t colors[4][4]={{255,0,0,255},{0,255,0,255},{0,0,255,255},{255,255,255,255}};
  size_t offset=header;
  for(unsigned part=0;part<4;++part)for(unsigned pixel=0;pixel<(part<2?1u:4u);++pixel){std::memcpy(b.data()+offset,colors[part],4);offset+=4;}
  const auto layout=readVtfTextureLayoutCandidate(b);if(layout.frameCount!=2||layout.mips[1].offset!=header||layout.mips[0].offset!=header+8)throw std::runtime_error("animated offsets");
  for(uint32_t level=0;level<2;++level)for(uint32_t frame=0;frame<2;++frame){const auto image=decodeVtfMipCandidate(b,level,frame);const unsigned part=level==1?frame:2+frame;for(size_t p=0;p<image.rgba.size();p+=4)if(std::memcmp(image.rgba.data()+p,colors[part],4))throw std::runtime_error("wrong frame pixels");++decoded;}
  try{decodeVtfMipCandidate(b,0,2);throw std::logic_error("invalid frame accepted");}catch(const std::runtime_error&){++rejected;}
  b.pop_back();try{readVtfTextureLayoutCandidate(b);throw std::logic_error("truncated animation accepted");}catch(const std::runtime_error&){++rejected;}
 }
 std::cout<<"animated VTF fixture validation decoded="<<decoded<<" rejected="<<rejected<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
