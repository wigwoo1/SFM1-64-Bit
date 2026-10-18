#include "material_gamma_table_candidate.h"
#include "material_sdk_tint_gamma_candidate.h"
#include <fstream>
#include <cstring>
#include <iostream>
#include <thread>
int main(){try{
 auto generated=materialGammaTableCandidate();std::array<float,256> native;
 std::ifstream input("outputs/native-fixed-gamma22-table.bin",std::ios::binary);
 input.read(reinterpret_cast<char*>(native.data()),sizeof(native));
 if(!input||input.peek()!=std::char_traits<char>::eof())throw std::runtime_error("native gamma fixture invalid");
 if(std::memcmp(native.data(),generated.data(),sizeof(native)))throw std::runtime_error("native gamma table bit mismatch");
 const auto&shared=materialGammaTableSharedCandidate();
 if(&shared!=&materialGammaTableSharedCandidate()||std::memcmp(shared.data(),native.data(),sizeof(native)))throw std::runtime_error("shared native table identity or bits mismatch");
 std::array<const std::array<float,256>*,4>addresses{};std::array<std::thread,4>threads;
 for(size_t i=0;i<threads.size();++i)threads[i]=std::thread([&,i]{addresses[i]=&materialGammaTableSharedCandidate();});
 for(auto&thread:threads)thread.join();for(auto*address:addresses)if(address!=&shared)throw std::runtime_error("thread gamma table copied");
 auto tint=materialSdkTintGammaToLinearCandidate({.5f,.95f,5.f},generated,NativeGammaIndexRounding::nearestEven);
 if(tint!=std::array<float,3>{native[128],1.f,5.f})throw std::runtime_error("generated table tint mismatch");
 unsigned saved=_mm_getcsr();unsigned rejected=0;
 for(unsigned mode=1;mode<4;++mode){
  _mm_setcsr((saved&~0x6000u)|(mode<<13));
  try{materialGammaTableSharedCandidate();}catch(const std::runtime_error&){++rejected;}
 }
 _mm_setcsr(saved);if(rejected!=3)throw std::runtime_error("unsupported generation rounding accepted");
 std::cout<<"PASS x64 generated/shared material gamma: all256 native entries bit-identical, tint conversion passed, shared identity across four threads, three unsupported rounding modes rejected\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
