#define NOMINMAX
#include <windows.h>
#include <float.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cstring>
#include "material_parameter_orchestration_candidate.h"
struct CompletionAction{uint32_t index=0,operation=0;std::array<uint32_t,4>bits{};};
#include "material_fallback_native_fixture.h"
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){std::fprintf(stderr,"fallback fault=%08lx eip=%08lx address=%08lx exe=%p shader=%p fs=%p eax=%08lx ecx=%08lx edx=%08lx\n",p->ExceptionRecord->ExceptionCode,p->ContextRecord->Eip,p->ExceptionRecord->NumberParameters>1?static_cast<unsigned long>(p->ExceptionRecord->ExceptionInformation[1]):0ul,GetModuleHandleA(nullptr),GetModuleHandleA("stdshader_dx9.dll"),GetModuleHandleA("filesystem_stdio.dll"),p->ContextRecord->Eax,p->ContextRecord->Ecx,p->ContextRecord->Edx);return EXCEPTION_CONTINUE_SEARCH;}
static void require(bool v,const char*m){if(!v)throw std::runtime_error(m);}
static uint32_t word(std::istream&in){uint32_t v=0;require(bool(in.read(reinterpret_cast<char*>(&v),4)),"Input word");return v;}
static std::string text(std::istream&in,unsigned max){auto n=word(in);require(n<=max,"Input text extent");std::string s(n,'\0');require(bool(in.read(s.data(),n)),"Input text");return s;}
static SourceMaterialPatchNodeCandidate node(std::istream&in,unsigned depth=0){require(depth<16,"Initial tree depth");SourceMaterialPatchNodeCandidate n;n.kind=word(in);n.bits=word(in);n.key=text(in,255);n.text=text(in,4095);auto count=word(in);require(count<256,"Initial children");for(unsigned i=0;i<count;++i)n.children.push_back(node(in,depth+1));return n;}
static void put(std::ostream&out,uint32_t n){out.write(reinterpret_cast<const char*>(&n),4);}
static void putText(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
static void putTree(std::ostream&out,const SourceMaterialPatchNodeCandidate&n){putText(out,n.key);put(out,n.kind);put(out,n.bits);putText(out,n.text);out.write(reinterpret_cast<const char*>(&n.pointer),8);put(out,uint32_t(n.children.size()));for(const auto&c:n.children)putTree(out,c);}
static uint32_t selectedIndex(void*root,void*selected){uint32_t index=0,result=UINT32_MAX;auto visit=[&](auto&&self,void*p)->void{if(p==selected)result=index;++index;void*c=nullptr;std::memcpy(&c,static_cast<uint8_t*>(p)+24,4);while(c){self(self,c);std::memcpy(&c,static_cast<uint8_t*>(c)+20,4);}};visit(visit,root);require(result!=UINT32_MAX,"Selected node belongs to final root");return result;}
static void putResult(std::ostream&out,MaterialFallbackNativeFixture&f,void*root,void*aggregate,void*selected){
    put(out,f.warnings+f.shaderWarnings);put(out,selectedIndex(root,selected));uintptr_t shader=0,array=0;std::memcpy(&shader,f.material.data()+0x14,4);std::memcpy(&array,f.material.data()+0x24,4);std::string shaderName;
    for(size_t i=0;i<f.shaders.size();++i)if(reinterpret_cast<uintptr_t>(f.shaders[i])==shader)shaderName=f.shaderNames[i];require(!shaderName.empty()&&array,"Final material fields");putText(out,shaderName);put(out,uint32_t(f.lookups.size()));for(const auto&s:f.lookups)putText(out,s);putTree(out,f.extract(root));putTree(out,f.extract(aggregate));put(out,f.material[0x22]);
    for(unsigned i=0;i<f.material[0x22];++i){auto*b=static_cast<uint8_t*>(reinterpret_cast<void**>(array)[i]);uintptr_t owner=0,data=0;std::memcpy(&owner,b+32,4);require(owner==reinterpret_cast<uintptr_t>(f.material.data()),"Actual material variable owner");std::array<uint32_t,28>q{};q[0]=1;std::memcpy(&q[1],b+28,4);q[1]&=0xffff;std::memcpy(&q[2],b+8,4);std::memcpy(q.data()+3,b+12,16);q[7]=(q[1]&15)==5?1u:(q[1]&15)==8?2u:0u;std::string value;using Name=const char*(__thiscall*)(void*);std::string name=reinterpret_cast<Name>(0x10017b80)(b);
        if((q[1]&15)==1){std::memcpy(&data,b+4,4);value=reinterpret_cast<const char*>(data);}if((q[1]&15)==7){std::memcpy(&data,b+36,4);require(data!=0,"Matrix storage");q[8]=1;q[9]=*reinterpret_cast<const uint8_t*>(data+64)!=0;std::memcpy(q.data()+12,reinterpret_cast<void*>(data),64);}q[10]=uint32_t(value.size());q[11]=uint32_t(name.size());out.write(reinterpret_cast<const char*>(q.data()),sizeof(q));out.write(value.data(),value.size());out.write(name.data(),name.size());
    }
}
int main(int argc,char**argv){try{
    require(argc==6,"Private material, library directory, disposable mount, input and fresh output required");require(!std::ifstream(argv[5],std::ios::binary).good(),"Fresh output required");std::ifstream in(argv[4],std::ios::binary);std::ofstream out(argv[5],std::ios::binary);require(bool(in)&&bool(out),"Streams");auto observer=AddVectoredExceptionHandler(1,&fault);MaterialFallbackNativeFixture native(argv[1],argv[2],argv[3]);
    unsigned oldControl=0,control=0;_controlfp_s(&oldControl,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);auto oldMxcsr=_mm_getcsr();_mm_setcsr(0x1f80);
    const auto count=word(in);put(out,count);for(unsigned i=0;i<count;++i){native.ownerName=text(in,255);bool prefix=word(in)!=0;auto source=node(in),error=node(in);std::cerr<<"case="<<i<<" build\n";SourceMaterialPatchNodeCandidate empty;empty.key="patchAccumulator";auto root=native.build(source),aggregate=native.build(empty),errorRoot=native.build(error);std::cerr<<"case="<<i<<" loop\n";
        auto selected=native.runTree(root,aggregate,errorRoot,{}, {},92,prefix);require(selected!=nullptr,"Selected tree");auto final=native.extract(root);auto acc=native.extract(aggregate);std::cout<<"case="<<i<<" final="<<final.key<<" warnings="<<native.warnings+native.shaderWarnings<<" lookups=";for(const auto&s:native.lookups)std::cout<<s<<',';std::cout<<" children="<<final.children.size()<<" alpha=";
        for(const auto&n:final.children)if(n.key=="$alpha")std::cout<<n.kind<<':'<<n.text<<':'<<n.bits;std::cout<<'\n';putResult(out,native,root,aggregate,selected);
        native.cleanupResult();native.destroyTree(root);native.destroyTree(aggregate);native.destroyTree(errorRoot);
    }
    require(in.peek()==std::char_traits<char>::eof(),"Trailing input");out.flush();require(bool(out),"Output write");native.finish();native.verifyCode();_mm_setcsr(oldMxcsr);_controlfp_s(&control,oldControl,_MCW_RC|_MCW_PC);RemoveVectoredExceptionHandler(observer);std::cout<<"PASS native_fallback_cases="<<count<<" allocations="<<native.allocations<<" releases="<<native.releases<<" null_frees="<<native.nullFrees<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
