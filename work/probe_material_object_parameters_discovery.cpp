#include "material_object_parameters_native_fixture.h"
static LONG CALLBACK parameterFault(EXCEPTION_POINTERS*p){std::fprintf(stderr,"object parameter fault=%08lx eip=%08lx address=%08lx\n",p->ExceptionRecord->ExceptionCode,p->ContextRecord->Eip,p->ExceptionRecord->NumberParameters>1?static_cast<unsigned long>(p->ExceptionRecord->ExceptionInformation[1]):0ul);return EXCEPTION_CONTINUE_SEARCH;}
int main(int argc,char**argv){try{
    need(argc==4,"Private material/library/mount");auto observer=AddVectoredExceptionHandler(1,&parameterFault);MaterialObjectParametersNativeFixture f(argv[1],argv[2],argv[3]);f.beginParameters();f.configure({}, {},92);
    SourceMaterialPatchNodeCandidate source;source.key="UnlitGeneric";source.children.push_back({"$alpha",1,".375"});source.children.push_back({"$color",1,"[.25 .5 1]"});auto*tree=f.build(source);auto*raw=f.createActual("fixture/object",tree);
    std::cout<<"before flags="<<(f.read(raw,0x20)&0xffff)<<" refs="<<f.read(raw,0x1c)<<std::endl;bool result=f.initializeActual(raw);
    std::cout<<"result="<<result<<" flags="<<(f.read(raw,0x20)&0xffff)<<" params="<<unsigned(raw[0x22])<<" same_source="<<(f.read(raw,0x7c)==reinterpret_cast<uintptr_t>(tree))<<" depth="<<f.tokens.size()<<" max_depth="<<f.maxDepth<<" forward=";
    for(size_t n=0x88;n<=0xa0;n+=4)std::cout<<std::hex<<f.read(raw,n)<<',';std::cout<<std::dec<<std::endl;
    need(result&&raw[0x22]==68,"Normal wrapper initialized Unlit");need(f.initializeActual(raw),"Already initialized wrapper");f.uncacheActual(raw);need(raw[0x22]==0&&!f.read(raw,0x24),"Real uncache clears vars");need(f.initializeActual(raw),"Reinitialize after uncache");f.destroyActual(raw);f.finishParameters();RemoveVectoredExceptionHandler(observer);
    std::cout<<"PASS actual_object_wrapper allocations="<<f.allocations<<" releases="<<f.releases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
