#include "source_bsp_inline_brush_candidate.h"
#include <iostream>

static void require(bool condition,const char*message){if(!condition)throw std::runtime_error(message);}
static void word(std::vector<uint8_t>&data,size_t offset,uint32_t value){std::memcpy(data.data()+offset,&value,4);}
static void number(std::vector<uint8_t>&data,size_t offset,float value){std::memcpy(data.data()+offset,&value,4);}
static void shortWord(std::vector<uint8_t>&data,size_t offset,uint16_t value){std::memcpy(data.data()+offset,&value,2);}
static uint32_t readWord(const std::vector<uint8_t>&data,size_t offset){uint32_t value;std::memcpy(&value,data.data()+offset,4);return value;}
using Color=std::array<uint8_t,4>;
static constexpr std::array<Color,4> firstStyle{{{16,32,48,0},{32,48,64,1},{64,80,96,255},{96,112,128,0}}};

// A complete minimal uncompressed BSP face, with independently placed lighting
// bytes. Padding contains decoys so a wrong address can remain in bounds.
static std::vector<uint8_t> fixture(int32_t lightOffset,uint8_t styleCount,bool truncate=false,bool bumped=false){
 std::vector<uint8_t>bytes(1036,0);std::memcpy(bytes.data(),"VBSP",4);word(bytes,4,20);
 auto lump=[&](uint32_t id,std::vector<uint8_t>data,uint32_t version=0){
  auto offset=static_cast<uint32_t>(bytes.size());word(bytes,8+id*16,offset);
  word(bytes,12+id*16,static_cast<uint32_t>(data.size()));word(bytes,16+id*16,version);
  bytes.insert(bytes.end(),data.begin(),data.end());
 };
 std::vector<uint8_t>vertices(36);number(vertices,12,2);number(vertices,24,1);number(vertices,28,1);lump(3,vertices);
 std::vector<uint8_t>edges(16);shortWord(edges,4,0);shortWord(edges,6,1);
 shortWord(edges,8,1);shortWord(edges,10,2);shortWord(edges,12,2);shortWord(edges,14,0);lump(12,edges);
 std::vector<uint8_t>surfedges(12);for(uint32_t i=0;i<3;++i)word(surfedges,i*4,i+1);lump(13,surfedges);
 std::vector<uint8_t>texinfo(72);number(texinfo,0,1);number(texinfo,20,1);
 number(texinfo,32,.5f);number(texinfo,52,.5f);if(bumped)word(texinfo,64,0x800);lump(6,texinfo);
 std::vector<uint8_t>texdata(32);for(size_t offset:{16u,20u,24u,28u})word(texdata,offset,64);lump(2,texdata);
 const std::string material="test/lightmap";std::vector<uint8_t>names(material.begin(),material.end());names.push_back(0);
 lump(43,names);lump(44,std::vector<uint8_t>(4));
 std::vector<uint8_t>face(56);shortWord(face,8,3);shortWord(face,12,UINT16_MAX);
 for(uint8_t i=0;i<4;++i)face[16+i]=i<styleCount?i:255;
 word(face,20,static_cast<uint32_t>(lightOffset));word(face,36,1);word(face,40,1);lump(7,face,1);
 std::vector<uint8_t>model(48);number(model,12,2);number(model,16,2);word(model,44,1);lump(14,model);
 std::vector<uint8_t>lighting(512,222);
 if(lightOffset>=0)for(uint8_t style=0;style<styleCount;++style)for(size_t i=0;i<4;++i){
  Color color=style?Color{8,16,24,0}:firstStyle[i];
  const auto offset=static_cast<size_t>(lightOffset)+style*(bumped?64:16)+i*4;
  std::copy(color.begin(),color.end(),lighting.begin()+offset);
 }
 if(truncate)lighting.resize(static_cast<size_t>(lightOffset)+(styleCount-1)*(bumped?64:16)+16-1);
 lump(8,lighting);return bytes;
}
static std::array<float,3> decode(Color color){
 const float scale=std::ldexp(1.f,static_cast<int8_t>(color[3]))/255.f;
 return {color[0]*scale,color[1]*scale,color[2]*scale};
}
static void checkColors(const SourceBspBrushGeometryCandidate&brush,uint8_t styles){
 require(brush.lightColors.size()==3,"synthetic face light channel count");
 const auto a=decode(firstStyle[0]),b=decode(firstStyle[1]),c=decode(firstStyle[2]),d=decode(firstStyle[3]);
 const auto extra=styles>1?decode({8,16,24,0}):std::array<float,3>{};
 for(size_t component=0;component<3;++component){
  const std::array<float,3>expected{a[component]+extra[component],b[component]+extra[component],
   (a[component]+b[component]+c[component]+d[component])*.25f+extra[component]};
  for(size_t vertex=0;vertex<3;++vertex)
   require(std::abs(brush.lightColors[vertex][component]-expected[vertex])<1e-6f,
           "BSP lightmap byte offset, style stride or bilinear sample mismatch");
 }
}
int main(){try{
 static_assert(sizeof(void*)==8,"x64 required");
 const auto baseline=readSourceBspInlineBrushCandidate(fixture(0,1),0);checkColors(baseline,1);
 for(int32_t offset:{16,40}){
  const auto relocated=readSourceBspInlineBrushCandidate(fixture(offset,1),0);checkColors(relocated,1);
  require(relocated.lightColors==baseline.lightColors,"lightmap relocation changed decoded values");
 }
 checkColors(readSourceBspInlineBrushCandidate(fixture(16,2),0),2);
 checkColors(readSourceBspInlineBrushCandidate(fixture(16,2,false,true),0),2);
 // An HDR-only map has a separate face table and lighting byte stream. Its
 // LDR records deliberately point to the wrong samples, as in the benchmark.
 auto hdr=fixture(16,1);
 const auto faceOffset=readWord(hdr,8+7*16);
 for(size_t component=0;component<4;++component){
  word(hdr,8+58*16+component*4,component==0?faceOffset:component==1?56:component==2?1:0);
 }
 const auto dummyOffset=static_cast<uint32_t>(hdr.size());
 std::vector<uint8_t>dummyFace(hdr.begin()+faceOffset,hdr.begin()+faceOffset+56);
 hdr.insert(hdr.end(),dummyFace.begin(),dummyFace.end());
 word(hdr,8+7*16,dummyOffset);word(hdr,dummyOffset+20,0);
 for(size_t component=0;component<4;++component){uint32_t value=0;std::memcpy(&value,hdr.data()+8+8*16+component*4,4);word(hdr,8+53*16+component*4,value);}
 word(hdr,12+8*16,0);
 checkColors(readSourceBspInlineBrushCandidate(hdr,0),1);
 auto invalidHdr=hdr;word(invalidHdr,16+58*16,2);bool hdrRejected=false;
 try{(void)readSourceBspInlineBrushCandidate(invalidHdr,0);}catch(const std::runtime_error&){hdrRejected=true;}
 require(hdrRejected,"invalid selected HDR face version accepted");
 auto ldrPreferred=fixture(16,1);
 for(size_t component=0;component<4;++component){
  word(ldrPreferred,8+58*16+component*4,readWord(ldrPreferred,8+7*16+component*4));
  word(ldrPreferred,8+53*16+component*4,readWord(ldrPreferred,8+8*16+component*4));
 }
 word(ldrPreferred,16+58*16,2); // unused HDR data cannot disturb selected LDR data
 checkColors(readSourceBspInlineBrushCandidate(ldrPreferred,0),1);
 auto legacyHdr=hdr;word(legacyHdr,12+58*16,0);word(legacyHdr,dummyOffset+20,16);
 checkColors(readSourceBspInlineBrushCandidate(legacyHdr,0),1);
 for(const auto&brush:{readSourceBspInlineBrushCandidate(fixture(-1,1),0),
                       readSourceBspInlineBrushCandidate(fixture(16,0),0)})
  for(const auto&color:brush.lightColors)require(color==std::array<float,3>{1,1,1},"unlit-face fallback changed");
 bool rejected=false;try{(void)readSourceBspInlineBrushCandidate(fixture(16,2,true),0);}
 catch(const std::runtime_error&){rejected=true;}require(rejected,"truncated face lighting accepted");
 std::cout<<"bsp_lightmap_addressing=passed offsets=0,16,40 relocation_exact=1 styles=1,2 "
             "bumped_style_stride=1 hdr_face_pairing=1 ldr_preference=1 legacy_hdr_fallback=1 hdr_version_guard=1 corner_and_bilinear_samples=1 "
             "signed_exponents=1 unlit_fallbacks=2 truncated_rejected=1\n";
 return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
