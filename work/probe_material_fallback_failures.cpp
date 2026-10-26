// Reuse the accepted private fixture and exact result framing, without changing
// the accepted probe or any owned runtime implementation.
#define main acceptedFallbackReferenceMain
#include "probe_material_fallback_reference.cpp"
#undef main

int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==4,"Private original reference only");
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    require(argc==7,"Mode, private material, libraries, disposable mount, request, fresh output required");
    const bool file=std::string(argv[1])=="--file";
    require(file||std::string(argv[1])=="--caller","Reference mode");
    require(!std::ifstream(argv[6],std::ios::binary).good(),"Fresh output required");
    std::ifstream in(argv[5],std::ios::binary);std::ofstream out(argv[6],std::ios::binary);
    require(bool(in)&&bool(out),"Streams");
    auto observer=AddVectoredExceptionHandler(1,&fault);
    MaterialFallbackNativeFixture native(argv[2],argv[3],argv[4]);
    unsigned oldControl=0,control=0;_controlfp_s(&oldControl,0,0);
    _controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);
    const auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);
    const auto count=word(in);require(count>0&&count<=512,"Case extent");put(out,count);
    for(unsigned i=0;i<count;++i){
        const auto name=text(in,255);const auto prefix=word(in);require(prefix<=1,"Prefix bool");
        auto source=node(in),second=node(in);auto root=native.build(source),other=native.build(second);
        if(file){
            native.warnings=0;
            // Original complete material file loader, including its original
            // handling of a failed nested patch load. No replacement parser.
            using Load=bool(__cdecl*)(void*,void*,const char*,bool,void*);
            const bool loaded=reinterpret_cast<Load>(0x10006ba0)(root,other,name.c_str(),prefix!=0,nullptr);
            put(out,loaded?1u:0u);put(out,native.warnings);
            std::vector<void*>peers;void*current=root;
            while(current){require(peers.size()<16,"Root peer extent");peers.push_back(current);std::memcpy(&current,static_cast<uint8_t*>(current)+20,4);}
            put(out,uint32_t(peers.size()));for(auto*p:peers)putTree(out,native.extract(p));
            putTree(out,native.extract(other));native.destroyTree(root);native.destroyTree(other);
        }else{
            native.ownerName=name;SourceMaterialPatchNodeCandidate empty;empty.key="patchAccumulator";
            auto aggregate=native.build(empty);
            auto selected=native.runTree(root,aggregate,other,{}, {},92,prefix!=0);
            require(selected!=nullptr,"Selected tree");putResult(out,native,root,aggregate,selected);
            native.cleanupResult();native.destroyTree(root);native.destroyTree(aggregate);native.destroyTree(other);
        }
        native.verifyCode();
    }
    require(in.peek()==std::char_traits<char>::eof(),"Trailing input");out.flush();require(bool(out),"Output write");
    native.finish();native.verifyCode();_mm_setcsr(oldMxcsr);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);
    RemoveVectoredExceptionHandler(observer);
    std::cout<<"PASS failure_reference_cases="<<count<<" mode="<<(file?"file":"caller")
             <<" allocations="<<native.allocations<<" releases="<<native.releases<<" null_frees="<<native.nullFrees<<" dev_messages="<<native.developerMessages<<'\n';
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
