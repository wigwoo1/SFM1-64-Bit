#define NOMINMAX
#include <windows.h>
#include <fstream>
#include <vector>
#include <array>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include "camera_rigid_inverse_candidate.h"
int main(int argc,char**argv){try{static_assert(sizeof(void*)==4,"native probe must be x86");std::ifstream f("D:\\SteamLibrary\\steamapps\\common\\SourceFilmmaker\\game\\sdktools\\python\\2.7\\win32\\Lib\\site-packages\\vs\\datamodel_pyd.pyd",std::ios::binary);std::vector<char>file((std::istreambuf_iterator<char>(f)),{});if(file.size()<4096)throw std::runtime_error("missing original DLL");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(file.data());auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(file.data()+dos->e_lfanew);if(nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("unexpected image base");auto base=static_cast<char*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),nt->OptionalHeader.SizeOfImage,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));if(base!=reinterpret_cast<char*>(0x10000000))throw std::runtime_error("preferred image address unavailable");auto section=IMAGE_FIRST_SECTION(nt);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){if(section[i].PointerToRawData+section[i].SizeOfRawData>file.size())throw std::runtime_error("invalid section");std::memcpy(base+section[i].VirtualAddress,file.data()+section[i].PointerToRawData,section[i].SizeOfRawData);}FlushInstructionCache(GetCurrentProcess(),base,nt->OptionalHeader.SizeOfImage);


 if(argc!=2||std::ifstream(argv[1]).good())throw std::runtime_error("fresh output required");std::ofstream out(argv[1],std::ios::binary);if(!out)throw std::runtime_error("output unavailable");using Native=void(__cdecl*)(const float*,float*);auto native=reinterpret_cast<Native>(0x10059750);unsigned count=0;
 for(float angle:{0.f,.3f,1.f,2.f,3.f})for(auto t:{std::array<float,3>{0,0,0},std::array<float,3>{10,20,30},std::array<float,3>{-58.3431f,-19.0557f,36.7978f},std::array<float,3>{9187.86f,-6932.94f,-4944.51f}}){float c=std::cos(angle),s=std::sin(angle);std::array<std::array<float,4>,3>matrix{{{c,-s,0,t[0]},{s,c,0,t[1]},{0,0,1,t[2]}}};std::array<float,12>expected;native(matrix[0].data(),expected.data());auto candidate=cameraRigidInverseCandidate(matrix);if(std::memcmp(candidate.rows.data(),expected.data(),48))throw std::runtime_error("native inverse mismatch");out.write(reinterpret_cast<const char*>(matrix.data()),48);out.write(reinterpret_cast<const char*>(expected.data()),48);++count;}out.close();if(!out)throw std::runtime_error("write failed");std::cout<<"native_camera_inverse_cases="<<count<<" bitwise_equal=1 DLL_entrypoint_not_called=1\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
