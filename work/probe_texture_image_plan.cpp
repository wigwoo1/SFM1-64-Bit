#include "texture_image_plan_candidate.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned native x64 plan");
    if(argc!=3||std::ifstream(argv[2]).good())throw std::runtime_error("query fresh-output");
    std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);
    if(!in||!out)throw std::runtime_error("probe IO");
    std::array<uint32_t,21>q{};uint32_t count=0;
    while(in.read(reinterpret_cast<char*>(q.data()),sizeof(q))){
        const auto before=q;TextureImagePlanInputCandidate v;
        v.major=q[0];v.minor=q[1];v.rawFlags=q[2];v.requestedFlags=q[3];
        v.width=q[4];v.height=q[5];v.frames=q[6];v.format=q[7];
        v.reflectivityBits={q[8],q[9],q[10]};v.picmip=q[11];v.maxWidth=q[12];
        v.maxHeight=q[13];v.dimensionClamp=q[14];v.forceFullMipChain=q[15]!=0;
        v.graffitiPolicy=q[16]!=0;v.lodResourceKnownAbsent=q[17]!=0;
        v.overridesKnownAbsent=q[18]!=0;v.hardwareKnown=q[19]!=0;
        TextureImagePlanCandidate plan;plan.fill(0xaabbccdd);
        const uint32_t ok=textureImagePlanCandidate(v,plan);
        if(q!=before||ok!=q[20])throw std::runtime_error("admission/input identity");
        if(!ok)for(auto n:plan)if(n!=0xaabbccdd)throw std::runtime_error("failed plan changed output");
        out.write(reinterpret_cast<const char*>(&ok),4);out.write(reinterpret_cast<const char*>(plan.data()),sizeof(plan));++count;
    }
    if(!in.eof()||in.gcount()||!count)throw std::runtime_error("framing");out.flush();if(!out)throw std::runtime_error("output");
    std::cout<<"PASS owned_image_plan_cases="<<count<<" original_runtime=ABSENT gpu_calls=0 editor=NOT_CONNECTED\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
