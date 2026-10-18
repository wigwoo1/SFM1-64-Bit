#define NOMINMAX
#include <windows.h>
#include "source_material_patch_policy_candidate.h"
#include "native_material_keyvalues_fixture.h"
#include <fstream>
#include <iostream>
template<class T>static T read(std::istream&in){T value;if(!in.read(reinterpret_cast<char*>(&value),sizeof(value)))throw std::runtime_error("format input span");return value;}
template<class T>static void put(std::ostream&out,T value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));}
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native formatter x86");if(argc!=4)throw std::runtime_error("copied-libraries input output required");NativeMaterialKeyValuesFixture native(argv[1]);std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("format streams");auto count=read<uint32_t>(in);if(count>100000)throw std::runtime_error("format extent");put(out,count);using Get=const char*(__thiscall*)(void*,const char*,const char*);for(uint32_t i=0;i<count;++i){SourceMaterialPatchNodeCandidate n;n.key="value";n.kind=3;n.bits=read<uint32_t>(in);auto value=native.build(n);const char*text=reinterpret_cast<Get>(native.at(0x71470))(value,nullptr,"");if(!text)throw std::runtime_error("format null");const std::string result(text);if(result.size()>63)throw std::runtime_error("original format storage extent");put(out,uint32_t(result.size()));out.write(result.data(),result.size());native.destroy(value);}out.flush();if(!out||in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("format wire span");native.verify();std::cout<<"PASS original include GetString float cases="<<count<<'\n';return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
