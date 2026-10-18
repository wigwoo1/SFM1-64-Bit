#pragma once
// Test-only private copy; never initializes DLL, loads imports or edits files.
// Kept separate from engine fixture whose image-size/identity gates differ.
struct NativePrivateMdlCacheFixture {
    uint8_t*base=nullptr;size_t size=0;
    std::vector<std::pair<size_t,size_t>>ranges;
    std::vector<std::pair<size_t,std::vector<uint8_t>>>code;
    explicit NativePrivateMdlCacheFixture(const char*path){
        std::ifstream f(path,std::ios::binary);if(!f)throw std::runtime_error("datacache read");const std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});
        if(b.size()<sizeof(IMAGE_DOS_HEADER))throw std::runtime_error("datacache DOS");const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(b.data());if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||size_t(dos->e_lfanew)+sizeof(IMAGE_NT_HEADERS32)>b.size())throw std::runtime_error("datacache PE span");const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(b.data()+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC||nt->OptionalHeader.ImageBase!=0x10000000)throw std::runtime_error("datacache PE identity");size=nt->OptionalHeader.SizeOfImage;if(size!=0x4a000)throw std::runtime_error("datacache pinned image size");const auto sections=IMAGE_FIRST_SECTION(nt);if(reinterpret_cast<const uint8_t*>(sections)+nt->FileHeader.NumberOfSections*sizeof(IMAGE_SECTION_HEADER)>b.data()+b.size())throw std::runtime_error("datacache sections");
        base=static_cast<uint8_t*>(VirtualAlloc(reinterpret_cast<void*>(0x10000000),size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(base!=reinterpret_cast<void*>(0x10000000))throw std::runtime_error("datacache preferred base unavailable");
        for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=sections[i];if(s.VirtualAddress>size||s.SizeOfRawData>size-s.VirtualAddress||s.PointerToRawData>b.size()||s.SizeOfRawData>b.size()-s.PointerToRawData)throw std::runtime_error("datacache section span");std::memcpy(base+s.VirtualAddress,b.data()+s.PointerToRawData,s.SizeOfRawData);if(s.Characteristics&IMAGE_SCN_MEM_EXECUTE)ranges.push_back({s.VirtualAddress,s.SizeOfRawData});}
    }
    void seal(){DWORD old;for(const auto&r:ranges){code.push_back({r.first,std::vector<uint8_t>(base+r.first,base+r.first+r.second)});if(!VirtualProtect(base+r.first,r.second,PAGE_EXECUTE_READ,&old))throw std::runtime_error("datacache code protection");}FlushInstructionCache(GetCurrentProcess(),base,size);}
    void verify()const{for(const auto&r:code)if(std::memcmp(base+r.first,r.second.data(),r.second.size()))throw std::runtime_error("datacache code changed");}
    ~NativePrivateMdlCacheFixture(){if(base)VirtualFree(base,0,MEM_RELEASE);}
};
