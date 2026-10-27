#define SFM64_NATIVE_OBJECT_PARAMETERS
#define SFM64_OBJECT_PARAMETER_MAIN scrollInitPriorParameterMain
#include "probe_material_object_parameters.cpp"
#undef SFM64_OBJECT_PARAMETER_MAIN
#include "material_texture_scroll_client_native_fixture.h"
int main(int argc,char**argv){try{
    gate(argc==4&&!std::ifstream(argv[3]).good(),"private-client query fresh-output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);gate(bool(in)&&bool(out),"Scroll Init IO");
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);auto handler=AddVectoredExceptionHandler(1,&lifetimeFault);MaterialTextureScrollClientNativeFixture f(argv[1]);f.configureScroll();
    const auto count=word(in);gate(count>0&&count<10000,"Initialization case count");put(out,count);unsigned successes=0,ignoredFailures=0;
    for(unsigned i=0;i<count;++i){auto declaration=node(in);MaterialObjectParameterStateCandidate seed;
        for(const auto&name:{"$out","$drive","$out[1]"}){MaterialTypedDispatchCandidate v;v.created=true;v.name=name;v.value.numeric.typeAndMetadata=0x22;v.value.numeric.vector={0x3e800000,0xbf000000,0,0};seed.material.parameters.values.push_back(v);}
        f.prepareTexture(seed);auto*p=f.createScroll("TextureScroll");const bool ok=f.initializeFunction(p,declaration);put(out,i);put(out,ok);
        if(ok){++successes;const char*keys[]{"textureScrollRate","textureScrollAngle","textureScale"};const uint32_t defaults[]{0x3f800000,0,0x3f800000};
            for(unsigned j=0;j<3;++j){std::array<uint32_t,3>record{};const auto*raw=static_cast<const uint8_t*>(p)+8+12*j;std::memcpy(&record[1],raw+4,4);if(record[1])std::memcpy(&record[2],raw+8,4);else std::memcpy(&record[0],raw,4);
                const bool bound=record[1]!=0;put(out,bound);put(out,bound?f.index(reinterpret_cast<void*>(record[1])):record[0]);put(out,bound?record[2]:0);
                SourceMaterialPatchNodeCandidate probe;probe.key="FloatInputProbe";const auto*field=sourceMaterialPatchFindConstCandidate(declaration,keys[j]);if(field){auto copy=*field;copy.key="input";probe.children.push_back(copy);}
                std::array<uint32_t,3>independent{0x12345678,0,0xa5a5a5a5};const bool returned=f.initializeInput(independent,probe,defaults[j]);put(out,returned);
                gate((independent[1]!=0)==bound,"Actual field input binding equals independent original FloatInput");if(bound)gate(independent[1]==record[1]&&independent[2]==record[2],"Actual field binding/component");else gate(returned&&independent[0]==record[0],"Defined original literal/default");if(!returned){gate(bound&&f.index(reinterpret_cast<void*>(record[1]))==f.parameterCount,"Ignored missing input points to shared cache; no uninitialized literal read");++ignoredFailures;}
            }
        }
        f.destroy(p);gate(f.blocks.empty(),"Original initialization cleanup");f.image.verify();
    }
    gate(in.peek()==std::char_traits<char>::eof(),"Complete Init query");out.flush();gate(bool(out),"Complete Init output");RemoveVectoredExceptionHandler(handler);std::cout<<"PASS scroll_initialization_cases="<<count<<" initialized="<<successes<<" ignored_input_failures="<<ignoredFailures<<" allocations="<<f.allocations<<" releases="<<f.releases<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
