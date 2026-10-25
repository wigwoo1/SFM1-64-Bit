#include "vtf_block_decode.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {
void require(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
void word(std::vector<uint8_t>&bytes,size_t offset,uint32_t value){std::memcpy(bytes.data()+offset,&value,4);}
void half(std::vector<uint8_t>&bytes,size_t offset,uint16_t value){std::memcpy(bytes.data()+offset,&value,2);}
struct Fixture {
 std::string name;
 uint32_t minor,header;
 bool thumbnail;
 std::vector<uint8_t>bytes;
 std::array<size_t,3>offsets;
};
// Independent fixed 4x2, 2-frame, 3-mip encoding. Payload lengths per frame
// are 32/8/4 bytes; disk order is small mip first, then animation frame.
const std::array<std::array<uint8_t,4>,6>colors{{
 {{20,40,60,255}},{{70,90,110,180}},
 {{120,140,160,130}},{{170,190,210,80}},
 {{220,30,50,40}},{{80,100,240,0}}
}};
Fixture make(uint32_t minor,uint32_t header,bool thumbnail){
 Fixture f{"v7-"+std::to_string(minor)+"-header"+std::to_string(header)+(thumbnail?"-thumb":"-no-thumb"),minor,header,thumbnail,{}, {}};
 const size_t high=header+(thumbnail?8u:0u);
 f.bytes.resize(high+88);word(f.bytes,0,0x00465456);word(f.bytes,4,7);word(f.bytes,8,minor);word(f.bytes,12,header);
 half(f.bytes,16,4);half(f.bytes,18,2);word(f.bytes,20,0x2040);half(f.bytes,24,2);word(f.bytes,52,0);f.bytes[56]=3;
 word(f.bytes,57,thumbnail?13u:UINT32_MAX);f.bytes[61]=f.bytes[62]=thumbnail?4:0;
 if(minor>=2)half(f.bytes,63,1);
 if(minor>=3){
  word(f.bytes,68,thumbnail?3u:1u);
  if(thumbnail){word(f.bytes,80,1);word(f.bytes,84,header);word(f.bytes,88,0x30);word(f.bytes,92,uint32_t(high));word(f.bytes,96,0x02435243);word(f.bytes,100,0xabcdef12);}
  else{word(f.bytes,80,0x30);word(f.bytes,84,uint32_t(high));}
 }
 if(thumbnail)for(size_t n=header;n<high;++n)f.bytes[n]=0xee;
 f.offsets={high+24,high+8,high};
 size_t offset=high;
 for(size_t level:{2u,1u,0u})for(size_t frame=0;frame<2;++frame){
  const size_t pixels=level==2?1:level==1?2:8;
  for(size_t pixel=0;pixel<pixels;++pixel){const auto&color=colors[level*2+frame];std::memcpy(f.bytes.data()+offset,color.data(),4);offset+=4;}
 }
 require(offset==f.bytes.size(),"owned fixture length");return f;
}
void check(const Fixture&f){
 const auto layout=readVtfTextureLayoutCandidate(f.bytes);
 require(layout.format==0&&layout.flags==0x2040&&layout.frameCount==2&&layout.mips.size()==3,"fixture layout metadata");
 const std::array<size_t,3>sizes{32,8,4};const std::array<uint32_t,3>widths{4,2,1},heights{2,1,1};
 for(uint32_t level=0;level<3;++level){
  const auto&mip=layout.mips[level];
  require(mip.level==level&&mip.offset==f.offsets[level]&&mip.size==sizes[level]&&mip.width==widths[level]&&mip.height==heights[level],"exact fixture mip addressing");
  for(uint32_t frame=0;frame<2;++frame){const auto image=decodeVtfMipCandidate(f.bytes,level,frame);
   require(image.width==widths[level]&&image.height==heights[level]&&image.rgba.size()==sizes[level],"decoded fixture dimensions");
   for(size_t pixel=0;pixel<image.rgba.size();pixel+=4)require(!std::memcmp(image.rgba.data()+pixel,colors[level*2+frame].data(),4),"exact fixture frame/mip pixel");
  }
 }
}
}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");
 require(argc==2||argc==3,"usage: test fresh-workspace-output-directory [--expect-legacy-bug]");
 const bool baseline=argc==3&&std::string(argv[2])=="--expect-legacy-bug";
 require(argc==2||baseline,"unknown test option");
 const auto directory=std::filesystem::absolute(argv[1]).lexically_normal();
 const auto allowed=(std::filesystem::current_path()/"outputs").lexically_normal();const auto relative=directory.lexically_relative(allowed);
 require(!relative.empty()&&!relative.is_absolute()&&*relative.begin()!="..","fixtures must be under workspace outputs");
 require(std::filesystem::create_directory(directory),"fresh fixture directory required");
 std::vector<Fixture>fixtures{make(1,64,false),make(1,80,false),make(2,80,false),make(2,96,false),make(1,64,true),make(2,80,true)};
 for(uint32_t minor:{3u,4u,5u}){fixtures.push_back(make(minor,88,false));fixtures.push_back(make(minor,104,true));}
 size_t baselineRejected=0,passed=0,rejected=0;
 for(const auto&f:fixtures){
  std::ofstream output(directory/(f.name+".vtf"),std::ios::binary);output.write(reinterpret_cast<const char*>(f.bytes.data()),std::streamsize(f.bytes.size()));output.close();require(bool(output),"fixture file write");
  if(baseline&&f.minor<=2&&!f.thumbnail){
   bool failed=false;try{(void)readVtfTextureLayoutCandidate(f.bytes);}catch(const std::runtime_error&error){
    const std::string message=error.what();require(message=="VTF resource header"||message=="VTF resource table bounds","unexpected baseline rejection");
    failed=true;++baselineRejected;std::cout<<f.name<<" old_parser_rejection="<<message<<'\n';
   }require(failed,"old parser unexpectedly accepted target fixture");continue;
  }
  check(f);++passed;
  if(baseline)continue;
  auto reject=[&](const std::vector<uint8_t>&bad){bool failed=false;try{(void)readVtfTextureLayoutCandidate(bad);}catch(const std::runtime_error&){failed=true;}require(failed,"malformed VTF accepted");++rejected;};
  // Every shorter byte sequence must fail, including partial mip/frame payloads.
  for(size_t n=0;n<f.bytes.size();++n)reject({f.bytes.begin(),f.bytes.begin()+n});
  auto bad=f.bytes;bad.push_back(0);reject(bad);
  for(size_t field:{61u,62u}){bad=f.bytes;bad[field]=f.thumbnail?0:1;reject(bad);}
  for(auto field:std::array<std::pair<size_t,uint32_t>,6>{{{4,8},{8,6},{12,f.minor==1?63u:79u},{20,0x4000},{52,999},{24,0}}}){bad=f.bytes;word(bad,field.first,field.second);reject(bad);}
  for(uint8_t count:std::array<uint8_t,2>{0,4}){bad=f.bytes;bad[56]=count;reject(bad);}
  if(f.minor>=3){
   bad=f.bytes;word(bad,80,0x12345678);reject(bad);
   bad=f.bytes;word(bad,68,33);reject(bad);
   bad=f.bytes;word(bad,f.thumbnail?92:84,f.header-1);reject(bad);
   if(f.thumbnail){
    bad=f.bytes;word(bad,80,0x30);reject(bad); // duplicate high-res entry
    bad=f.bytes;word(bad,88,0x02444f4c);reject(bad); // missing high-res entry
    bad=f.bytes;word(bad,84,uint32_t(f.offsets[2]));reject(bad); // overlapping thumbnail
   }
  }
  for(auto selection:std::array<std::pair<uint32_t,uint32_t>,2>{{{3,0},{0,2}}}){
   bool failed=false;try{(void)decodeVtfMipCandidate(f.bytes,selection.first,selection.second);}catch(const std::runtime_error&){failed=true;}require(failed,"out-of-range selection accepted");++rejected;
  }
 }
 if(baseline){require(baselineRejected==4&&passed==8,"baseline coverage");std::cout<<"PASS_BASELINE_REPRODUCTION old_no_thumbnail_rejected=4 existing_cases_passed=8\n";}
 else{require(passed==12,"positive fixture coverage");std::cout<<"PASS legacy_no_thumbnail_and_regressions fixtures="<<passed<<" decoded_frame_mips="<<passed*6<<" rejected="<<rejected<<"\n";}
 return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
