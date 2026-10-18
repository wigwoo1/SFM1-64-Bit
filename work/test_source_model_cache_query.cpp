#include "source_model_cache_query_candidate.h"
#include <iostream>
#include <limits>
struct SafetyCacheServices {
    using LightingState=std::array<uintptr_t,2>;
    bool control=false,isValid=false,disableAfterFirst=false;unsigned validCalls=0,calls=0;std::array<uint32_t,2>flags{};
    bool enabled(){return control;}
    bool valid(uintptr_t){++validCalls;return isValid;}
    LightingState cached(uintptr_t handle,uint32_t f,uintptr_t&value){flags.at(calls++)=f;value=uintptr_t(0x567800002222ull);if(disableAfterFirst)control=false;return {handle,value};}
    uintptr_t point(const std::array<float,3>&,LightingState&state,uintptr_t&aux,uintptr_t renderable,uint32_t f,bool special){flags.at(calls++)=f;aux=uintptr_t(0x678900003333ull);state={renderable,special};if(disableAfterFirst)control=false;return uintptr_t(0x789a00004444ull);}
};
int main(){try{
    static_assert(sizeof(uintptr_t)==8,"AMD64 policy test");unsigned checks=0;auto require=[](bool ok){if(!ok)throw std::runtime_error("cache routing safety assertion");};
    for(const auto&p:std::array<std::array<std::string_view,3>,8>{{{{"Alpha_BETA_End","beta","1"}},{{"Alpha","ALPHA","1"}},{{"Alpha","zzz","0"}},{{"","x","0"}},{{"x","","0"}},{{"","","0"}},{{"abABac","ABAC","1"}},{{"xxFCVAR_NEVER_AS_STRINGyy","fcvar_never_as_string","1"}}}}){require(sourceCacheFilterContainsCandidate(p[0],p[1])==(p[2]=="1"));++checks;}
    SourceModelCacheQuerySnapshotCandidate s;s.point={1,2,3};s.renderable=uintptr_t(0x123400001111ull);s.diagnosticMode=5;s.instanceRecordPresent=true;s.instanceName="prefix_PYRO_suffix";s.filterString="pyro";
    SafetyCacheServices point;const auto p=sourceModelCacheQueryCandidate(s,point);require(p.positionScratch==s.point&&p.filterCompared&&point.flags[0]==15&&p.state[0]==s.renderable&&p.state[1]==1&&p.value==0x789a00004444ull&&p.auxiliary==0x678900003333ull);++checks;
    SafetyCacheServices retry;retry.control=true;retry.disableAfterFirst=true;const auto r=sourceModelCacheQueryCandidate(s,retry);require(retry.calls==2&&retry.flags==std::array<uint32_t,2>{6,15}&&r.state[1]==1);++checks;
    s.cachedHandle=0; s.point[0]=std::numeric_limits<float>::quiet_NaN();SafetyCacheServices cached;const auto c=sourceModelCacheQueryCandidate(s,cached,uintptr_t(0xabcd00005555ull));require(!c.positionScratch&&!c.filterCompared&&cached.calls==1&&cached.validCalls==0&&cached.flags[0]==7&&c.auxiliary==0xabcd00005555ull&&c.value==0x567800002222ull);++checks;
    SafetyCacheServices cachedRetry;cachedRetry.control=true;cachedRetry.disableAfterFirst=true;const auto cr=sourceModelCacheQueryCandidate(s,cachedRetry);require(cachedRetry.calls==2&&cachedRetry.validCalls==1&&cachedRetry.flags==std::array<uint32_t,2>{6,7});++checks;
    s.cachedHandle.reset();SafetyCacheServices bad;bool rejected=false;try{sourceModelCacheQueryCandidate(s,bad);}catch(const std::runtime_error&){rejected=true;}require(rejected&&bad.calls==0);++checks;
    std::cout<<"PASS model cache query safety checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
