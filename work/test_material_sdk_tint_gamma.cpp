#include "material_sdk_tint_gamma_candidate.h"
#include <iostream>
#include <limits>
#include <immintrin.h>
#include <fstream>
int main(){try{
 std::array<float,256>table;for(size_t n=0;n<table.size();++n)table[n]=float(n)/1024.f;
 auto midpoint=materialSdkTintGammaToLinearCandidate({0,.5f,5},table,NativeGammaIndexRounding::nearestEven);
 if(midpoint!=std::array<float,3>{0,table[128],5})throw std::runtime_error("lookup or HDR bypass wrong");
 auto boundary=materialSdkTintGammaToLinearCandidate({.95f,1,std::nextafter(1.f,2.f)},table,NativeGammaIndexRounding::nearestEven);
 if(boundary!=std::array<float,3>{1,1,std::nextafter(1.f,2.f)})throw std::runtime_error("threshold or HDR boundary wrong");
 float below=std::nextafter(.95f,0.f);auto low=materialSdkTintGammaToLinearCandidate({below,below,below},table,NativeGammaIndexRounding::nearestEven);if(low[0]!=table[242])throw std::runtime_error("below threshold index wrong");
 unsigned saved=_mm_getcsr();
 for(unsigned ambient=0;ambient<4;++ambient){
  _mm_setcsr((saved&~0x6000u)|(ambient<<13));
  for(unsigned policy=0;policy<4;++policy){
   auto converted=materialSdkTintGammaToLinearCandidate({.5f,1.f,5.f},table,static_cast<NativeGammaIndexRounding>(policy));
   unsigned index=(policy==0||policy==2)?128:127;
   if(converted!=std::array<float,3>{table[index],1.f,5.f}){_mm_setcsr(saved);throw std::runtime_error("explicit tint rounding or HDR boundary wrong");}
   if((_mm_getcsr()&0x6000u)!=(ambient<<13)){_mm_setcsr(saved);throw std::runtime_error("tint changed ambient rounding control");}
  }
 }
 _mm_setcsr(saved);
 std::array<float,256>nativeTable;
 std::ifstream fixture("outputs/native-fixed-gamma22-table.bin",std::ios::binary);
 fixture.read(reinterpret_cast<char*>(nativeTable.data()),sizeof(nativeTable));
 if(!fixture||fixture.peek()!=std::char_traits<char>::eof())throw std::runtime_error("native table fixture missing or malformed");
 auto nativeTint=materialSdkTintGammaToLinearCandidate({.5f,.95f,5.f},nativeTable,NativeGammaIndexRounding::nearestEven);
 if(nativeTint!=std::array<float,3>{nativeTable[128],1.f,5.f}||nativeTable[128]<=0||nativeTable[128]>=.5f)throw std::runtime_error("native table tint conversion wrong");
 unsigned rejected=0;for(float value:{-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}){try{materialSdkTintGammaToLinearCandidate({value,0,0},table,NativeGammaIndexRounding::nearestEven);}catch(const std::runtime_error&){++rejected;}}
 table[0]=std::numeric_limits<float>::infinity();try{materialSdkTintGammaToLinearCandidate({0,0,0},table,NativeGammaIndexRounding::nearestEven);}catch(const std::runtime_error&){++rejected;}
 if(rejected!=4)throw std::runtime_error("invalid tint/table accepted");
 std::cout<<"PASS SDK tint policy: explicit rounding across sixteen mode combinations, gamma lookup index, .95/1 boundaries, HDR bypass, four malformed rejections; native startup oracle table tint tested; installed selfillum/GPU parity unverified\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
