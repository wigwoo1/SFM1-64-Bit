#pragma once
// Test-only PE copy. Never calls DllMain or resolves/loads engine imports.
// The caller patches only explicit host work-pool/memset dependencies before
// sealing executable sections and recording their bytes for integrity checks.
struct NativePrivateWorldImageFixture {
 uint8_t*base=nullptr;size_t size=0;
 std::vector<std::pair<size_t,size_t>>ranges;
 std::vector<std::pair<size_t,std::vector<uint8_t>>>codeBefore;
 explicit NativePrivateWorldImageFixture(const char*path){
  std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("engine read");std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});
  if(b.size()<sizeof(IMAGE_DOS_HEADER))throw std::runtime_error("PE header");auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(b.data());if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||size_t(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>b.size())throw std::runtime_error("PE offset");auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(b.data()+dos->e_lfanew);
  if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt->OptionalHeader.ImageBase!=0x10000000||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC)throw std::runtime_error("PE identity");size=nt->OptionalHeader.SizeOfImage;if(size>0x2000000||size<0x638000)throw std::runtime_error("PE image size");
  const auto sections=IMAGE_FIRST_SECTION(nt);if(reinterpret_cast<const uint8_t*>(sections)+nt->FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER)>b.data()+b.size())throw std::runtime_error("PE section table");
  base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("native base unavailable");
  for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=sections[i];if(s.VirtualAddress>size||s.SizeOfRawData>size-s.VirtualAddress||s.PointerToRawData>b.size()||s.SizeOfRawData>b.size()-s.PointerToRawData)throw std::runtime_error("PE section span");std::memcpy(base+s.VirtualAddress,b.data()+s.PointerToRawData,s.SizeOfRawData);if(s.Characteristics&IMAGE_SCN_MEM_EXECUTE)ranges.push_back({s.VirtualAddress,s.SizeOfRawData});}
 }
 void seal(){DWORD old;for(const auto&r:ranges){codeBefore.push_back({r.first,std::vector<uint8_t>(base+r.first,base+r.first+r.second)});if(!VirtualProtect(base+r.first,r.second,PAGE_EXECUTE_READ,&old))throw std::runtime_error("native code protection");}FlushInstructionCache(GetCurrentProcess(),base,size);}
 void verify()const{for(const auto&r:codeBefore)if(std::memcmp(base+r.first,r.second.data(),r.second.size()))throw std::runtime_error("private original code changed");}
};
