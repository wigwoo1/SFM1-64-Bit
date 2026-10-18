#include "studio_color_table_candidate.h"
#include <fstream>
#include <cstdio>
int main(){static_assert(sizeof(void*)==8,"x64 required");
 std::array<float,256> native{};
 std::ifstream input("outputs/native-color-table-candidate.bin",std::ios::binary);
 input.read(reinterpret_cast<char*>(native.data()),1024);if(!input||input.peek()!=EOF)return 1;
 const auto candidate=studioColorTableCandidate();unsigned mismatches=0;uint64_t digest=1469598103934665603ull;
 for(unsigned i=0;i<256;++i){uint32_t actual,expected;std::memcpy(&actual,&candidate[i],4);std::memcpy(&expected,&native[i],4);
  if(actual!=expected){++mismatches;std::printf("table_mismatch index=%u native=%08x candidate=%08x\n",i,expected,actual);}
  digest=(digest^actual)*1099511628211ull;
 }
 std::printf("x64_color_table_entries=256 bit_mismatches=%u digest=%llu full_generator_parity=unverified\n",mismatches,static_cast<unsigned long long>(digest));return mismatches?2:0;
}
