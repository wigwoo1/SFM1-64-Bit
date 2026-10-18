#include "read_binary_asset_candidate.h"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <set>

struct BspLumpCandidate { uint32_t offset=0, size=0; int32_t version=0, fourCC=0; };
static uint32_t u32(const std::vector<uint8_t>& b,size_t p){if(p>b.size()||4>b.size()-p)throw std::runtime_error("BSP header range");uint32_t v;std::memcpy(&v,b.data()+p,4);return v;}
static int32_t i32(const std::vector<uint8_t>& b,size_t p){int32_t v;auto x=u32(b,p);std::memcpy(&v,&x,4);return v;}
static uint16_t u16(const std::vector<uint8_t>& b,size_t p){if(p>b.size()||2>b.size()-p)throw std::runtime_error("BSP face range");uint16_t v;std::memcpy(&v,b.data()+p,2);return v;}
static int16_t i16(const std::vector<uint8_t>& b,size_t p){int16_t v;auto x=u16(b,p);std::memcpy(&v,&x,2);return v;}

int main(int argc,char**argv){
 try {
  if(argc!=3)throw std::runtime_error("usage: inspect_source_bsp_inline_model map.bsp *model");
  std::string token=argv[2];if(token.size()<2||token[0]!='*')throw std::runtime_error("inline model token required");
  char*end=nullptr;auto parsed=std::strtoull(token.c_str()+1,&end,10);if(!end||*end||parsed>SIZE_MAX)throw std::runtime_error("invalid inline model id");size_t requested=size_t(parsed);if(!requested)throw std::runtime_error("inline model ids are one-based");
  auto b=readBinaryAssetCandidate(argv[1]);if(b.size()<1036||std::memcmp(b.data(),"VBSP",4))throw std::runtime_error("unsupported BSP header");
  uint32_t version=u32(b,4);std::array<BspLumpCandidate,64> lumps{};
  for(size_t n=0;n<lumps.size();++n){size_t p=8+n*16;lumps[n]={u32(b,p),u32(b,p+4),i32(b,p+8),i32(b,p+12)};auto&l=lumps[n];if(l.offset>b.size()||l.size>b.size()-l.offset)throw std::runtime_error("BSP lump outside file");}
  auto models=lumps[14],faces=lumps[7];if(models.size%48)throw std::runtime_error("BSP model lump alignment");if(faces.version!=1||faces.size%56)throw std::runtime_error("unsupported BSP face encoding");size_t count=models.size/48;
  size_t index=requested-1;if(index>=count)throw std::runtime_error("inline model index outside BSP model lump");size_t m=models.offset+index*48;int32_t first=i32(b,m+40),faceCount=i32(b,m+44);
  if(first<0||faceCount<0||uint64_t(first)+uint64_t(faceCount)>faces.size/56)throw std::runtime_error("BSP inline face range");
  size_t displacements=0;uint64_t edges=0;std::set<int32_t> materials;
  for(int32_t n=0;n<faceCount;++n){size_t f=faces.offset+size_t(first+n)*56;int16_t edgeCount=i16(b,f+8),material=i16(b,f+10),disp=i16(b,f+12);if(edgeCount<0)throw std::runtime_error("negative BSP face edge count");edges+=uint16_t(edgeCount);materials.insert(material);displacements+=disp!=-1;}
  std::cout<<"bsp_version="<<version<<" model_count="<<count<<" token="<<token<<" indexing=one_based_model_lump_index="<<index<<" first_face="<<first<<" face_count="<<faceCount<<" polygon_edges="<<edges<<" displacement_faces="<<displacements<<" texinfo_ids=";
  bool comma=false;for(auto id:materials){if(comma)std::cout<<',';std::cout<<id;comma=true;}std::cout<<"\n";
  return 0;
 } catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
