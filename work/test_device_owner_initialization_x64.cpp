#include <array>
#include <fstream>
#include <cstring>
#include <cstdio>
#include "studio_device_owner_initialization_candidate.h"
// Original-layout serialization backend is for oracle comparison only.
// It does not define the application's x64 owner layout.
struct Backend{
 std::array<unsigned char,0x2d0> storage;unsigned baseCalls=0,clearCalls=0;
 void word(unsigned offset,int32_t value){std::memcpy(storage.data()+offset,&value,4);}
 void base(){++baseCalls;word(8,0x12345678);word(0x6c,42);}
 void installDerivedIdentity(){word(0,0);}
 void descriptorWord(unsigned index,int32_t value){word(0x70+index*4,value);}
 void clearDescriptor(){uint32_t value;std::memcpy(&value,storage.data()+0x74,4);if(baseCalls!=1||value!=1)throw 1;++clearCalls;std::memset(storage.data()+0x70,0,0x38);}
 void set(StudioDeviceOwnerFieldCandidate field,int32_t value){constexpr unsigned offsets[]{0x2c4,0x2c8,0x2c,0xb4,0xb8,0x6c,0x2a0,0xc4,0xa8,0xb0,0x178,0x17c,0x180,0x184,0x188,0x2a8,0x2cc};word(offsets[static_cast<unsigned>(field)],value);}
 void clearPairByte(unsigned index){storage[0xbc+index]=0;}
 void keepUpperFlags(){storage[0xac]&=0xc0;}
 void clearFinalByte(){storage[0xc8]=0;}
};
int main(){static_assert(sizeof(void*)==8,"x64 required");std::ifstream oracle("outputs/native-device-owner-constructor.bin",std::ios::binary);if(!oracle)return 1;
 for(unsigned pattern=0;pattern<256;++pattern){Backend backend{};backend.storage.fill(static_cast<unsigned char>(pattern));studioDeviceOwnerInitializationCandidate(backend);std::array<unsigned char,0x2d0> expected{};oracle.read(reinterpret_cast<char*>(expected.data()),expected.size());if(!oracle||expected!=backend.storage||backend.baseCalls!=1||backend.clearCalls!=1){std::printf("owner_initialization_mismatch=%u\n",pattern);return 2;}}
 if(oracle.peek()!=std::char_traits<char>::eof())return 3;
 std::printf("x64_native_owner_initialization_record_matches=256 normalized_field_policy=passed owner_layout_binding=pending\n");
}
