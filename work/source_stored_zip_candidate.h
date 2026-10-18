#pragma once
#include "source_material_keyvalues_files_candidate.h"
#include <array>
#include <map>
#include <set>
// Checked stored ZIP32 reader (PKWARE APPNOTE 6.3.10 sections4.3.7/12/16).
// No extraction/writes. Rejects compression, encryption, descriptors, ZIP64,
// multi-disk, ambiguous names/overlaps and unsupported file kinds explicitly.
inline uint32_t sourceStoredZipCrcCandidate(const uint8_t*data,size_t size){
 static const auto table=[](){std::array<uint32_t,256>t{};for(uint32_t i=0;i<256;++i){uint32_t v=i;for(unsigned bit=0;bit<8;++bit)v=(v>>1)^((v&1)?0xedb88320u:0u);t[i]=v;}return t;}();uint32_t result=0xffffffffu;for(size_t i=0;i<size;++i)result=(result>>8)^table[(result^data[i])&255];return result^0xffffffffu;
}
class SourceStoredZipCandidate {
 std::vector<uint8_t>data;struct Entry{size_t offset=0,size=0;};std::map<std::string,Entry>entries;
 void span(size_t off,size_t size)const{if(off>data.size()||size>data.size()-off)throw std::runtime_error("ZIP record span");}
 uint16_t u16(size_t p)const{span(p,2);return uint16_t(uint16_t(data[p])|(uint16_t(data[p+1])<<8));}
 uint32_t u32(size_t p)const{span(p,4);return uint32_t(data[p])|(uint32_t(data[p+1])<<8)|(uint32_t(data[p+2])<<16)|(uint32_t(data[p+3])<<24);}
 std::string string(size_t p,size_t n)const{span(p,n);return std::string(reinterpret_cast<const char*>(data.data()+p),n);}
 void extra(size_t p,size_t n)const{span(p,n);const size_t end=p+n;while(p<end){if(end-p<4)throw std::runtime_error("ZIP extra header");auto id=u16(p),size=u16(p+2);p+=4;if(size>end-p||id==1||id==0x9901||id==0x7075)throw std::runtime_error("ZIP extra extent/unsupported extension");p+=size;}}
 void parse(){
  if(data.size()<22||data.size()>(1u<<26))throw std::runtime_error("ZIP archive extent");size_t end=SIZE_MAX;const size_t lower=data.size()>65557?data.size()-65557:0;for(size_t p=data.size()-22;;--p){if(u32(p)==0x06054b50u&&size_t(u16(p+20))==data.size()-p-22){end=p;break;}if(p==lower)break;}if(end==SIZE_MAX)throw std::runtime_error("ZIP end record missing");
  const auto count=u16(end+10);if(u16(end+4)||u16(end+6)||u16(end+8)!=count||count==65535)throw std::runtime_error("ZIP disk/count/ZIP64 unsupported");const size_t central=u32(end+16),length=u32(end+12);if(central>end||length!=end-central)throw std::runtime_error("ZIP central directory span");size_t p=central;std::vector<std::pair<size_t,size_t>>localSpans;std::set<std::string>names;
  for(size_t i=0;i<count;++i){if(p>end||end-p<46||u32(p)!=0x02014b50u)throw std::runtime_error("ZIP central record");const auto needed=u16(p+6),flags=u16(p+8),method=u16(p+10),nameLength=u16(p+28),extraLength=u16(p+30),commentLength=u16(p+32);const size_t compressed=u32(p+20),size=u32(p+24),local=u32(p+42);const auto crc=u32(p+16),attributes=u32(p+38);const size_t extent=46+size_t(nameLength)+extraLength+commentLength;if(extent>end-p)throw std::runtime_error("ZIP central record extent");if(needed>20||(flags&~uint16_t(0x800))||method||u16(p+34)||compressed!=size||size>(1u<<24))throw std::runtime_error("ZIP entry compression/flags/extent unsupported");
   auto name=string(p+46,nameLength);bool directory=!name.empty()&&name.back()=='/';if(directory)name.pop_back();const auto unixKind=(attributes>>16)&0xf000u;if(unixKind&&unixKind!=0x8000u&&unixKind!=0x4000u)throw std::runtime_error("ZIP special file unsupported");directory=directory||unixKind==0x4000u||(attributes&0x10u);if(directory&&(size||crc))throw std::runtime_error("ZIP directory payload");const auto key=sourceMaterialKeyValuesPathCandidate(name);if(sourceMaterialPatchLowerCandidate(name)!=key||!names.insert(key).second)throw std::runtime_error("ZIP ambiguous path identity");extra(p+46+nameLength,extraLength);
   if(local>central||central-local<30||u32(local)!=0x04034b50u)throw std::runtime_error("ZIP local record span");const auto localName=u16(local+26),localExtra=u16(local+28);const size_t header=30+size_t(localName)+localExtra;if(header>central-local||size>central-local-header)throw std::runtime_error("ZIP local data extent");if(u16(local+4)!=needed||u16(local+6)!=flags||u16(local+8)!=method||u32(local+14)!=crc||u32(local+18)!=compressed||u32(local+22)!=size||string(local+30,localName)!=string(p+46,nameLength))throw std::runtime_error("ZIP local/central identity mismatch");extra(local+30+localName,localExtra);const size_t offset=local+header;if(sourceStoredZipCrcCandidate(data.data()+offset,size)!=crc)throw std::runtime_error("ZIP file CRC mismatch");localSpans.push_back({local,offset+size});if(!directory)entries.emplace(key,Entry{offset,size});p+=extent;
  }
  if(p!=end)throw std::runtime_error("ZIP central count/span mismatch");std::sort(localSpans.begin(),localSpans.end());size_t next=0;for(const auto&s:localSpans){if(s.first!=next||s.second<s.first)throw std::runtime_error("ZIP overlapping/unindexed local records");next=s.second;}if(next!=central)throw std::runtime_error("ZIP local records/central boundary mismatch");
 }
public:
 explicit SourceStoredZipCandidate(std::vector<uint8_t>bytes):data(std::move(bytes)){parse();}
 std::optional<std::vector<uint8_t>>read(const std::string&path)const{const auto key=sourceMaterialKeyValuesPathCandidate(path);auto i=entries.find(key);if(i==entries.end())return {};return std::vector<uint8_t>(data.begin()+i->second.offset,data.begin()+i->second.offset+i->second.size);}
 std::vector<std::string>names()const{std::vector<std::string>out;for(const auto&e:entries)out.push_back(e.first);return out;}
};
