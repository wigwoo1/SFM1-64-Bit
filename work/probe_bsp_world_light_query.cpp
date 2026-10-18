#include "source_bsp_world_light_trace_candidate.h"
#include "source_bsp_displacement_collision_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
// Same explicit test-only wire as probe_source_world_light_visibility.cpp.
struct Fixture {std::array<uint32_t,7>h{};std::array<float,6>p{};std::array<std::array<float,3>,6>v{};};
struct Result {float falloff=0;std::array<float,3>incoming{};float attenuation=0;std::array<uint32_t,8>events{};std::array<float,3>start{},delta{},debugStart{},debugEnd{};};
static_assert(sizeof(Fixture)==124&&sizeof(Result)==100,"explicit comparison wire");
static int32_t signedWord(uint32_t x){int32_t y;std::memcpy(&y,&x,4);return y;}
struct Services {
    const Fixture&f;Result&r;const SourceBspWorldLightTraceCandidate&world;
    int32_t style(uint32_t i){if(i!=f.h[2])throw std::runtime_error("BSP lighting style index");return signedWord(f.h[3]);}
    float intensityThreshold(){return f.p[4];}
    int32_t debugMode(){return signedWord(f.h[4]);}
    void debugLine(const std::array<float,3>&s,const std::array<float,3>&e){++r.events[1];r.events[7]=1;r.debugStart=s;r.debugEnd=e;}
    SourceWorldLightTraceCandidate trace(const SourceWorldLightRayCandidate&ray,uint32_t mask,const SourceWorldLightFilterCandidate&filter){
        const auto hit=world.trace(ray,mask,filter);
        // Compare newly computed geometry with the separately executed original
        // geometry stage. Never return the recorded fixture values as answers.
        if(std::memcmp(&hit.fraction,&f.p[5],4)||hit.surfaceFlags!=f.h[5]||std::memcmp(hit.endPosition.data(),f.v[5].data(),12))throw std::runtime_error("actual BSP trace differs from original geometry stage");
        ++r.events[0];r.events[2]=uint32_t(filter.mode);r.events[3]=uint32_t(filter.excludedEntity);r.events[4]=mask;r.events[5]=1;r.events[6]=ray.swept;r.start=ray.start;r.delta=ray.delta;return hit;
    }
};
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned AMD64");if(argc!=4)throw std::runtime_error("map queries output required");std::ifstream map(argv[1],std::ios::binary),in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!map||!in||!out)throw std::runtime_error("BSP lighting fixture IO");std::vector<uint8_t>b((std::istreambuf_iterator<char>(map)),{});if(b.size()<1036)throw std::runtime_error("BSP lighting header");uint32_t length;std::memcpy(&length,b.data()+8+6*16+4,4);if(length%72||length/72>=65535)throw std::runtime_error("BSP lighting texinfo extent");std::vector<uint32_t>binding(length/72);for(size_t i=0;i<binding.size();++i)binding[i]=uint32_t(binding.size()-1-i);SourceBspTraceSurfaceFlagsCandidate surfaces(b,binding);SourceBspBrushOnlyTraceCandidate brushes(b,binding);SourceBspDisplacementCollisionCandidate loaded(b,0);SourcePreparedWorldRayCandidate geometry(brushes,loaded.meshes);SourceBspWorldLightTraceCandidate world(geometry,surfaces);
    size_t count=0;Fixture f;while(in.read(reinterpret_cast<char*>(&f),sizeof(f))){if(f.h[1]>15||(f.h[1]&4)||f.h[2]>255||f.h[5]>65535||f.p[5]<0||f.p[5]>1)throw std::runtime_error("unsupported/malformed BSP lighting fixture");for(float x:f.p)if(!std::isfinite(x))throw std::runtime_error("nonfinite BSP lighting scalar");for(const auto&v:f.v)for(float x:v)if(!std::isfinite(x))throw std::runtime_error("nonfinite BSP lighting vector");const auto before=f;SourceWorldLightQueryCandidate light;light.angular.type=signedWord(f.h[0]);light.style=f.h[2];light.radius=f.p[0];light.constant=f.p[1];light.linear=f.p[2];light.quadratic=f.p[3];light.position=f.v[0];light.angular.intensity=f.v[1];light.angular.normal=f.v[2];std::array<float,3>delta{};for(unsigned i=0;i<3;++i)delta[i]=light.position[i]-f.v[3][i];Result result;result.falloff=sourceWorldLightDistanceCandidate(light,delta,(f.h[1]&2)!=0);Services services{f,result,world};const auto query=sourceWorldLightVisibilityQueryCandidate(light,f.v[3],f.h[1],f.h[6],services,f.v[4]);result.incoming=query.incoming;result.attenuation=query.attenuation;if(std::memcmp(&f,&before,sizeof(f)))throw std::runtime_error("BSP lighting input mutation");out.write(reinterpret_cast<const char*>(&result),sizeof(result));++count;}
    if(!in.eof()||in.gcount()!=0||!count)throw std::runtime_error("BSP lighting fixture truncation");out.flush();if(!out)throw std::runtime_error("BSP lighting output");std::cout<<"PASS actual BSP world light queries="<<count<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
