#include "vtf_block_decode.h"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("texture path required");
 std::ifstream f(argv[1],std::ios::binary);if(!f)throw std::runtime_error("texture read");
 std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});
 const auto layout=readVtfTextureLayoutCandidate(b);size_t pixels=0;
 for(const auto& mip:layout.mips){const auto image=decodeVtfMipCandidate(b,mip.level);if(image.width!=mip.width||image.height!=mip.height||image.rgba.size()!=size_t(image.width)*image.height*4)throw std::runtime_error("mip pixel size mismatch");pixels+=image.rgba.size();}
 unsigned rejected=0;
 auto reject=[&](const std::vector<uint8_t>&bad){try{readVtfTextureLayoutCandidate(bad);}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("malformed texture accepted");};
 auto write=[](std::vector<uint8_t>&v,size_t offset,uint32_t value){std::memcpy(v.data()+offset,&value,4);};
 auto bad=b;bad.resize(80);reject(bad);
 bad=b;bad.pop_back();reject(bad);
 bad=b;bad.push_back(0);reject(bad);
 if(b.size()>=104){uint32_t version,resources;std::memcpy(&version,b.data()+8,4);std::memcpy(&resources,b.data()+68,4);
  if(version>=3&&resources==3){
   uint32_t crc;std::memcpy(&crc,b.data()+96,4);
   if(crc==0x02435243){
    bad=b;write(bad,96,0x00435243);reject(bad);
    bad=b;write(bad,96,0x02345678);reject(bad);
    bad=b;write(bad,80,0x02435243);reject(bad);
    bad=b;write(bad,92,0);reject(bad);
    bad=b;write(bad,100,0xffffffff);readVtfTextureLayoutCandidate(bad); // inline metadata is not an offset
   }
  }
 }
 std::cout<<"VTF resource validation passed mips="<<layout.mips.size()<<" rgba_bytes="<<pixels<<" malformed_rejected="<<rejected<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
