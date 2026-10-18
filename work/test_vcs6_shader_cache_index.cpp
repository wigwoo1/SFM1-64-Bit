#include "vcs6_shader_cache_index_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"x64 required");
    if(argc!=2)throw std::runtime_error("manifest required");
    std::ifstream manifest(argv[1]);std::string path;unsigned cases=0,rejections=0;
    while(std::getline(manifest,path)){
        std::ifstream input(path,std::ios::binary);
        if(!input)throw std::runtime_error("cache open");
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
        unsigned count;manifest>>count;manifest.ignore();
        if(count==0){bool rejected=false;try{Vcs6ShaderCacheIndexCandidate bad(bytes);}catch(const std::runtime_error&){rejected=true;}
            if(!rejected)throw std::runtime_error("malformed index accepted");++rejections;continue;}
        Vcs6ShaderCacheIndexCandidate cache(bytes);
        for(unsigned i=0;i<count;++i){uint32_t arg,dyn,norm,resolved,begin,end;
            manifest>>arg>>dyn>>norm>>resolved>>begin>>end;manifest.ignore();
            auto selected=cache.select(arg,dyn);
            if(selected.normalized!=norm||selected.resolved!=resolved||selected.begin!=begin||selected.end!=end)
                throw std::runtime_error("Python oracle range mismatch");++cases;
        }
        bool rejected=false;try{cache.select(UINT32_MAX,0);}catch(const std::runtime_error&){rejected=true;}
        if(!rejected)throw std::runtime_error("argument range accepted");++rejections;
        rejected=false;try{cache.select(0,UINT32_MAX);}catch(const std::runtime_error&){rejected=true;}
        if(!rejected)throw std::runtime_error("dynamic range accepted");++rejections;
    }
    if(!manifest.eof()||!cases)throw std::runtime_error("manifest parse");
    std::cout<<"x64_index_selections="<<cases<<" rejected="<<rejections<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
