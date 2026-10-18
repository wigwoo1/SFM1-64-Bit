#include "source_bsp_leaf_ambient_candidate.h"
#include <iostream>
#include <limits>
static void word(std::vector<uint8_t>&b,size_t p,uint32_t v){for(size_t i=0;i<4;++i)b[p+i]=uint8_t(v>>(i*8));}
static void half(std::vector<uint8_t>&b,size_t p,uint16_t v){b[p]=uint8_t(v);b[p+1]=uint8_t(v>>8);}
static constexpr size_t leafOffset=1036,hdrIndex=1132,ldrIndex=1144,hdrSamples=1156,ldrSamples=1212;
static std::vector<uint8_t>fixture(){
 std::vector<uint8_t>b(1268);word(b,0,0x50534256);word(b,4,20);
 auto lump=[&](size_t id,size_t offset,size_t bytes,uint32_t version){word(b,8+id*16,uint32_t(offset));word(b,12+id*16,uint32_t(bytes));word(b,16+id*16,version);};
 lump(10,leafOffset,96,1);lump(51,hdrIndex,12,0);lump(52,ldrIndex,12,0);lump(55,hdrSamples,56,1);lump(56,ldrSamples,56,1);
 for(size_t i=0;i<3;++i)for(size_t axis=0;axis<3;++axis){half(b,leafOffset+i*32+8+axis*2,uint16_t(i*100));half(b,leafOffset+i*32+14+axis*2,uint16_t(i*100+255));}
 // Leaf0 redirects to leaf1. Its sample position must use leaf1 bounds.
 half(b,hdrIndex+2,1);half(b,hdrIndex+4,2);half(b,hdrIndex+6,0);half(b,hdrIndex+10,2);
 half(b,ldrIndex,1);half(b,ldrIndex+2,1);half(b,ldrIndex+6,0);half(b,ldrIndex+10,2);
 for(size_t sample=0;sample<2;++sample)for(size_t face=0;face<6;++face){size_t p=hdrSamples+sample*28+face*4;b[p]=uint8_t(16+face);b[p+1]=32;b[p+2]=64;b[p+3]=sample?uint8_t(255):uint8_t(1);b[ldrSamples+sample*28+face*4]=1;}
 b[hdrSamples+24]=0;b[hdrSamples+25]=127;b[hdrSamples+26]=255;
 b[hdrSamples+28+24]=255;b[hdrSamples+28+25]=0;b[hdrSamples+28+26]=128;return b;
}
int main(){try{
 const auto original=fixture();SourceBspLeafAmbientCandidate hdr(original,SourceBspAmbientEncodingCandidate::Hdr),ldr(original,SourceBspAmbientEncodingCandidate::Ldr);
 if(hdr.samples.size()!=2||hdr.leaves.size()!=3||hdr.leaves[0].sampleOwner!=std::optional<uint32_t>(1)||hdr.leaves[2].sampleOwner)throw std::runtime_error("ambient redirect/empty leaf");
 if(hdr.samplePosition(0,0)!=std::array<double,3>{100,227,355}||hdr.samplePosition(0,1)!=std::array<double,3>{355,100,228})throw std::runtime_error("ambient owner-relative positions");
 if(ldr.leaves[0].sampleOwner!=std::optional<uint32_t>(0)||ldr.samples[1].cube[0][0]!=1||hdr.samples[1].cube[0][0]!=16)throw std::runtime_error("HDR/LDR pairing");
 for(size_t face=0;face<6;++face)if(hdr.samples[0].cube[face][0]!=16+face||hdr.samples[1].cube[face][0]!=16+face||hdr.samples[0].cube[face][3]!=1||hdr.samples[1].cube[face][3]!=255)throw std::runtime_error("six-face raw ordering");
 auto chainBytes=original;half(chainBytes,hdrIndex+4,0);half(chainBytes,hdrIndex+6,2);half(chainBytes,hdrIndex+8,1);half(chainBytes,hdrIndex+10,0);
 SourceBspLeafAmbientCandidate chain(chainBytes,SourceBspAmbientEncodingCandidate::Hdr);
 if(chain.leaves[0].sampleOwner!=std::optional<uint32_t>(2)||chain.samplePosition(0,0)!=std::array<double,3>{200,327,455})throw std::runtime_error("chained redirect owner bounds");
 if(SourceBspLeafAmbientCandidate::rgbExpVector(hdr.samples[0].cube[0])!=std::array<double,3>{32,64,128}||SourceBspLeafAmbientCandidate::rgbExpVector(hdr.samples[1].cube[0])!=std::array<double,3>{8,16,32})throw std::runtime_error("signed exponent/vector units");
 auto low=SourceBspLeafAmbientCandidate::rgbExpVector({1,0,255,128}),high=SourceBspLeafAmbientCandidate::rgbExpVector({255,0,1,127});
 if(low[0]!=std::ldexp(1.0,-128)||!std::isfinite(high[0]))throw std::runtime_error("exponent endpoints");
 size_t rejected=0;auto reject=[&](const auto&bytes){try{SourceBspLeafAmbientCandidate invalid(bytes,SourceBspAmbientEncodingCandidate::Hdr);}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("malformed ambient data accepted");};
 auto b=original;b.resize(100);reject(b);
 b=original;word(b,4,19);reject(b);
 b=original;word(b,8+55*16,0xfffffff0);reject(b);
 b=original;word(b,12+55*16,55);reject(b);
 b=original;word(b,20+55*16,1);reject(b);
 b=original;word(b,16+10*16,0);reject(b);
 b=original;word(b,12+51*16,8);reject(b);
 b=original;half(b,hdrIndex+2,3);reject(b);
 b=original;half(b,hdrIndex+4,3);reject(b);
 b=original;half(b,hdrIndex+4,0);half(b,hdrIndex+6,0);reject(b);
 b=original;half(b,leafOffset+14,uint16_t(-1));reject(b);
 b=original;word(b,8+55*16,100);reject(b);
 try{(void)hdr.samplePosition(2,0);}catch(const std::runtime_error&){++rejected;}
 try{(void)hdr.samplePosition(0,2);}catch(const std::runtime_error&){++rejected;}
 if(rejected!=14||original!=fixture())throw std::runtime_error("ambient invalid/read-only checks");
 std::cout<<"PASS: HDR/LDR separation, redirects/self-empty, owner-relative sample positions, six raw faces, signed exponent/vector units, 14 invalid cases; no lighting interpolation claimed\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
