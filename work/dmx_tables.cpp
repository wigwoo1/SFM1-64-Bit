// Reconstructed binary-v5 string/element-table reader, not a full DMX loader.
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>
struct Reader {
 std::ifstream f; uint64_t size;
 explicit Reader(const char* path):f(path,std::ios::binary) {
  if(!f) throw std::runtime_error("open failed");
  f.seekg(0,std::ios::end);size=static_cast<uint64_t>(f.tellg());f.seekg(0);
 }
 void bytes(char* p,size_t n){if(!f.read(p,n))throw std::runtime_error("truncated input");}
 uint32_t u32(){unsigned char b[4];bytes(reinterpret_cast<char*>(b),4);return uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);}
 std::string cstr(){std::string s;char c;while(true){bytes(&c,1);if(!c)return s;if(s.size()>=1048576)throw std::runtime_error("string too long");s+=c;}}
 uint32_t count(){auto n=u32();if(n>size||n>10000000)throw std::runtime_error("invalid count");return n;}
 uint32_t blobCount(){auto n=u32();auto pos=f.tellg();if(pos<0||uint64_t(pos)>size||n>size-uint64_t(pos)||n>256u*1024u*1024u)throw std::runtime_error("invalid binary byte count");return n;}
 uint8_t u8(){char b;bytes(&b,1);return static_cast<uint8_t>(b);}
 void skip(uint64_t n){auto pos=static_cast<uint64_t>(f.tellg());if(n>size-pos)throw std::runtime_error("truncated value");f.seekg(static_cast<std::streamoff>(n),std::ios::cur);}
};
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: dmx_tables session.dmx");
 static_assert(sizeof(void*)==8,"native x64 build required");
 Reader r(argv[1]);auto header=r.cstr();
 if(header!="<!-- dmx encoding binary 5 format sfm_session 20 -->\n" && header!="<!-- dmx encoding binary 5 format sfm_session 20 -->")throw std::runtime_error("unsupported header");
 auto ns=r.count();std::vector<std::string> strings;strings.reserve(ns);
 for(uint32_t i=0;i<ns;++i)strings.push_back(r.cstr());
 auto ne=r.count();std::map<std::string,uint32_t> types;
 for(uint32_t i=0;i<ne;++i){auto type=r.u32(),name=r.u32();if(type>=ns||name>=ns)throw std::runtime_error("invalid string index");char id[16];r.bytes(id,16);++types[strings[type]];}
 auto attributeOffset=r.f.tellg();uint64_t attributes=0,references=0;std::map<unsigned,uint64_t> attributeTypes;
 for(uint32_t i=0;i<ne;++i){auto na=r.count();attributes+=na;for(uint32_t j=0;j<na;++j){
  if(r.u32()>=ns)throw std::runtime_error("invalid attribute name index");
  auto type=r.u8();++attributeTypes[type];bool array=type>=15;unsigned scalar=array?type-14:type;
  if(scalar<1||scalar>14)throw std::runtime_error("unknown attribute type "+std::to_string(type));
  auto n=array?r.count():1;
  for(uint32_t k=0;k<n;++k){
   if(scalar==1){auto ref=r.u32();if(ref==0xfffffffe){r.cstr();}else if(ref!=0xffffffff&&ref>=ne)throw std::runtime_error("invalid element reference");++references;}
   else if(scalar==5){if(array)r.cstr();else if(r.u32()>=ns)throw std::runtime_error("invalid value string index");}
   else if(scalar==6)r.skip(r.count());
   else {static const unsigned widths[]={0,0,4,4,1,0,0,4,4,8,12,16,12,16,64};r.skip(widths[scalar]);}
  }
 }}
 auto end=r.f.tellg();if(static_cast<uint64_t>(end)!=r.size)throw std::runtime_error("trailing bytes "+std::to_string(r.size-static_cast<uint64_t>(end)));
 std::cout<<"{\"pointer_bits\":64,\"strings\":"<<ns<<",\"elements\":"<<ne<<",\"attribute_offset\":"<<attributeOffset<<",\"file_bytes\":"<<r.size<<",\"distinct_types\":"<<types.size()<<",\"attributes\":"<<attributes<<",\"references\":"<<references<<",\"scope\":\"structural_scan\"}\n";
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
