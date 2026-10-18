#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
struct VtfMipCandidate {uint32_t level,width,height;size_t offset,size;};
struct VtfTextureLayoutCandidate {uint32_t format=0,flags=0,frameCount=0;std::vector<VtfMipCandidate>mips;};
// PC VTF7.1-7.5, animated 2D RGBA8888/BGRA8888 or block-compressed candidate.
// VTF7.3+ image resources are supported; inline CRC/LOD records are retained only as metadata.
inline VtfTextureLayoutCandidate readVtfTextureLayoutCandidate(const std::vector<uint8_t>&bytes){
 auto range=[&](uint64_t o,uint64_t n){if(o>bytes.size()||n>bytes.size()-o)throw std::runtime_error("VTF span");};
 auto word=[&](size_t o){range(o,4);uint32_t v;std::memcpy(&v,bytes.data()+o,4);return v;};auto half=[&](size_t o){range(o,2);uint16_t v;std::memcpy(&v,bytes.data()+o,2);return uint32_t(v);};
 auto blockBytes=[](uint32_t format)->uint32_t{if(format==13||format==20)return 8;if(format==14||format==15)return 16;throw std::runtime_error("unsupported VTF block format "+std::to_string(format));};
 auto imageSize=[&](uint32_t w,uint32_t h,uint32_t format){if(format==0||format==12)return uint64_t(w)*h*4;if(format==3)return uint64_t(w)*h*3;return uint64_t((w+3)/4)*((h+3)/4)*blockBytes(format);};
 range(0,65);if(word(0)!=0x00465456||word(4)!=7||(word(8)!=1&&word(8)!=2&&word(8)!=3&&word(8)!=4&&word(8)!=5))throw std::runtime_error("VTF7.1-7.5 required");auto version=word(8);auto header=word(12),width=half(16),height=half(18),flags=word(20);if(header<(version==1?64u:80u))throw std::runtime_error("VTF header size");range(0,header);if(!width||!height||!half(24)||(version>1&&!half(63))||(flags&0x4000))throw std::runtime_error("VTF animated 2D required");auto format=word(52);if(format!=0&&format!=3&&format!=12)blockBytes(format);auto count=bytes[56];uint32_t maximum=1;for(auto dimension=width>height?width:height;dimension>1;dimension>>=1)++maximum;if(!count||count>maximum)throw std::runtime_error("VTF mip count");
 uint64_t offset=header;auto lowWidth=bytes[61],lowHeight=bytes[62];if((lowWidth==0)!=(lowHeight==0))throw std::runtime_error("VTF low-res dimensions");
 if(version<=2&&lowWidth){auto lowSize=imageSize(lowWidth,lowHeight,word(57));range(offset,lowSize);offset+=lowSize;}
 else{if(header<80)throw std::runtime_error("VTF resource header");auto resources=word(68);if(!resources||resources>32||uint64_t(80)+uint64_t(resources)*8>header)throw std::runtime_error("VTF resource table bounds");bool highSeen=false,lowSeen=false,crcSeen=false;uint64_t lowOffset=0,lowSize=0;
 for(uint32_t n=0;n<resources;++n){auto type=word(80+size_t(n)*8),data=word(84+size_t(n)*8);if(type==0x30){if(highSeen||data<header)throw std::runtime_error("VTF image resource");highSeen=true;offset=data;}else if(type==1){if(lowSeen||!lowWidth||data<header)throw std::runtime_error("VTF thumbnail resource");lowSeen=true;lowOffset=data;lowSize=imageSize(lowWidth,lowHeight,word(57));range(lowOffset,lowSize);}else if(type==0x02435243){if(crcSeen)throw std::runtime_error("duplicate VTF CRC resource");crcSeen=true;/* Inline CRC metadata; checksum verification remains unimplemented. */}else if(type==0x02444f4c){/* LOD metadata is inline and does not change stored image dimensions or offsets. */}else throw std::runtime_error("unsupported VTF resource type");}
 if(!highSeen||bool(lowWidth)!=lowSeen)throw std::runtime_error("missing VTF image resource");
 uint64_t highSize=0;for(uint32_t level=0;level<count;++level){auto w=width>>level,h=height>>level;highSize+=imageSize(w?w:1,h?h:1,format);}highSize*=half(24);range(offset,highSize);if(lowSeen&&lowOffset<offset+highSize&&offset<lowOffset+lowSize)throw std::runtime_error("overlapping VTF image resources");
 }
 VtfTextureLayoutCandidate result;result.format=format;result.flags=flags;result.frameCount=half(24);result.mips.resize(count);for(uint32_t n=count;n>0;--n){auto level=n-1;uint32_t w=width>>level,h=height>>level;if(!w)w=1;if(!h)h=1;auto size=imageSize(w,h,format);range(offset,size*result.frameCount);result.mips[level]={level,w,h,size_t(offset),size_t(size)};offset+=size*result.frameCount;}if(offset!=bytes.size())throw std::runtime_error("VTF trailing data");return result;
}
