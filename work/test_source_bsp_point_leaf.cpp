#include "source_bsp_point_leaf_candidate.h"
#include <iostream>
#include <limits>
static void word(std::vector<uint8_t>&b,size_t p,uint32_t value){for(size_t i=0;i<4;++i)b[p+i]=uint8_t(value>>(i*8));}
static void number(std::vector<uint8_t>&b,size_t p,float value){uint32_t bits;std::memcpy(&bits,&value,4);word(b,p,bits);}
static constexpr size_t planes=1036,nodes=1076,leaves=1140,models=1236;
static std::vector<uint8_t>fixture(){
 std::vector<uint8_t>b(1284);word(b,0,0x50534256);word(b,4,20);
 auto lump=[&](unsigned id,size_t p,size_t length,uint32_t version){word(b,8+id*16,uint32_t(p));word(b,12+id*16,uint32_t(length));word(b,16+id*16,version);};
 lump(1,planes,40,0);lump(5,nodes,64,0);lump(10,leaves,96,1);lump(14,models,48,0);
 number(b,planes,1);number(b,planes+12,10);word(b,planes+16,0);
 number(b,planes+20,.5f);number(b,planes+24,.5f);number(b,planes+32,2);word(b,planes+36,3);
 word(b,nodes,0);word(b,nodes+4,uint32_t(-1));word(b,nodes+8,1);
 word(b,nodes+32,1);word(b,nodes+36,uint32_t(-2));word(b,nodes+40,uint32_t(-3));word(b,models+36,0);return b;
}
int main(){try{
 const auto base=fixture();SourceBspPointLeafCandidate tree(base);
 if(tree.leafAt({10,0,0})!=0||tree.leafAt({std::nextafter(10.f,0.f),0,0})!=1||tree.leafAt({0,4,0})!=1||tree.leafAt({0,3,0})!=2||tree.leafAt({100000,0,0})!=0)throw std::runtime_error("axial/general/tie/outside query");
 auto b=base;word(b,models+36,1);SourceBspPointLeafCandidate subroot(b);if(subroot.leafAt({20,0,0})!=1)throw std::runtime_error("model headnode ignored");
 b=base;word(b,models+36,uint32_t(-3));SourceBspPointLeafCandidate direct(b);if(direct.leafAt({0,0,0})!=2)throw std::runtime_error("leaf-only root");
 size_t rejected=0;auto reject=[&](const auto&bytes){try{SourceBspPointLeafCandidate invalid(bytes);}catch(const std::runtime_error&){++rejected;return;}throw std::runtime_error("invalid tree accepted");};
 b=base;word(b,nodes+4,0);reject(b);
 b=base;word(b,nodes+4,2);reject(b);
 b=base;word(b,nodes+4,0x80000000);reject(b);
 b=base;word(b,nodes,2);reject(b);
 b=base;number(b,planes,std::numeric_limits<float>::quiet_NaN());reject(b);
 b=base;number(b,planes,-1);reject(b);
 b=base;word(b,models+36,2);reject(b);
 b=base;word(b,12+5*16,63);reject(b);
 b=base;word(b,20+1*16,1);reject(b);
 b=base;word(b,16+10*16,0);reject(b);
 b=base;b.resize(1100);reject(b);
 b=base;word(b,planes+16,6);reject(b);
 try{(void)tree.leafAt({0,std::numeric_limits<float>::infinity(),0});}catch(const std::runtime_error&){++rejected;}
 if(rejected!=13)throw std::runtime_error("invalid tree coverage");
 std::cout<<"PASS: axial/general planes, exact/near ties, model root, negative leaf root, outside points and13 invalid inputs; SDK convention only\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
