#pragma once
// Test-only. Requires Windows/stream/vector definitions from the oracle.
// Normal initialization of workspace copies, not a private-PE engine shim.
struct NativeVphysicsLibraryFixture {
    HMODULE tier=nullptr,module=nullptr;void*object=nullptr;uintptr_t*table=nullptr;
    std::vector<std::pair<const uint8_t*,std::vector<uint8_t>>>sealedCode;
    explicit NativeVphysicsLibraryFixture(const char*directory){
        const std::string dir=directory;if(!SetDllDirectoryA(directory))throw std::runtime_error("physics DLL directory");
        tier=LoadLibraryExA((dir+"\\tier0.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!tier)throw std::runtime_error("copied physics tier0 load");
        module=LoadLibraryExA((dir+"\\vphysics.dll").c_str(),nullptr,LOAD_WITH_ALTERED_SEARCH_PATH);if(!module)throw std::runtime_error("copied physics library load");
        using Factory=void*(__cdecl*)(const char*,int*);const auto factory=reinterpret_cast<Factory>(GetProcAddress(module,"CreateInterface"));if(!factory)throw std::runtime_error("physics factory export");int code=-1;object=factory("VPhysicsCollision007",&code);if(!object||code)throw std::runtime_error("physics collision interface");table=*reinterpret_cast<uintptr_t**>(object);
        const auto base=reinterpret_cast<uint8_t*>(module);const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(base);const auto nt=reinterpret_cast<const IMAGE_NT_HEADERS32*>(base+dos->e_lfanew);for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){const auto&s=IMAGE_FIRST_SECTION(nt)[i];if(s.Characteristics&IMAGE_SCN_MEM_EXECUTE)sealedCode.push_back({base+s.VirtualAddress,std::vector<uint8_t>(base+s.VirtualAddress,base+s.VirtualAddress+s.Misc.VirtualSize)});}
    }
    uintptr_t rva(uintptr_t pointer)const{return pointer-reinterpret_cast<uintptr_t>(module);}
    void*address(size_t offset)const{return reinterpret_cast<uint8_t*>(module)+offset;}
    // Test-only call bridge, after verifying its unmodified target. The original
    // callee still executes; the bridge observes its results before returning.
    void observeCall(size_t offset,size_t expectedTarget,void*bridge){
        verify();auto*p=static_cast<uint8_t*>(address(offset));int32_t previous;std::memcpy(&previous,p+1,4);if(*p!=0xe8||reinterpret_cast<uintptr_t>(p+5)+previous!=reinterpret_cast<uintptr_t>(address(expectedTarget)))throw std::runtime_error("physics observer call target");
        DWORD old;if(!VirtualProtect(p,5,PAGE_EXECUTE_READWRITE,&old))throw std::runtime_error("physics observer protection");const int32_t next=int32_t(reinterpret_cast<uintptr_t>(bridge)-reinterpret_cast<uintptr_t>(p+5));std::memcpy(p+1,&next,4);DWORD ignored;if(!VirtualProtect(p,5,old,&ignored))throw std::runtime_error("physics observer restore protection");FlushInstructionCache(GetCurrentProcess(),p,5);
        for(auto&s:sealedCode)if(p>=s.first&&p<s.first+s.second.size())std::memcpy(s.second.data()+size_t(p-s.first),p,5);
    }
    void verify()const{for(const auto&s:sealedCode)if(std::memcmp(s.first,s.second.data(),s.second.size()))throw std::runtime_error("original loaded physics code changed");}
    ~NativeVphysicsLibraryFixture(){if(module)FreeLibrary(module);if(tier)FreeLibrary(tier);SetDllDirectoryA(nullptr);}
};
