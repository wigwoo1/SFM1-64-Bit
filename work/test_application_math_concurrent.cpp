#include <thread>
#include <future>
#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "studio_application_math_candidate.h"
static void check(bool b){if(!b)std::abort();}
int main(){std::promise<void> start;auto gate=start.get_future().share();std::array<std::thread,8> threads;std::array<const StudioMathTablesOwnerCandidate*,8> owners{};
 for(unsigned i=0;i<threads.size();++i)threads[i]=std::thread([&,i]{gate.wait();for(unsigned n=0;n<64;++n){studioInitializeApplicationMathCandidate();auto& owner=studioApplicationMathOwnerCandidate();check(owner.startup.initialized&&owner.trigonometric[64]==1.0f&&owner.gamma.textureGamma[255]==255);for(auto& block:owner.seeded)check(block.first==&block.values[92]&&block.second==&block.values[216]);owners[i]=&owner;}});
 start.set_value();for(auto& thread:threads)thread.join();auto& owner=studioApplicationMathOwnerCandidate();for(auto pointer:owners)check(pointer==&owner);
 std::ifstream trig("outputs/native-actual-sine-table.bin",std::ios::binary);std::array<unsigned,768> originalTrig{};trig.read(reinterpret_cast<char*>(originalTrig.data()),sizeof(originalTrig));check(bool(trig)&&trig.peek()==std::char_traits<char>::eof());for(unsigned i=0;i<256;++i)check(!std::memcmp(&owner.trigonometric[i],&originalTrig[3*i+2],4));
 std::ifstream seed("outputs/native-seeded-generator.bin",std::ios::binary);std::array<unsigned,7168> originalSeed{};seed.seekg(2*sizeof(originalSeed));seed.read(reinterpret_cast<char*>(originalSeed.data()),sizeof(originalSeed));check(bool(seed));for(unsigned i=0;i<32;++i)check(!std::memcmp(owner.seeded[i].values.data(),originalSeed.data()+i*224,880));
 std::puts("application_math_concurrent_first_startup_threads=8 requests=512 native_sine_seed_match=passed shared_owner_stable=passed");
}
