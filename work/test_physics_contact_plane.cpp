#include "physics_contact_plane_test_fixture.h"
#include <iostream>
#include <limits>
int main(){try{
    unsigned checks=0;auto require=[&](bool ok){if(!ok)throw std::runtime_error("contact plane assertion");++checks;};PhysicsContactPlaneInputFixture f;f.data.box.axes={{{{1,-1}},{{2,-2}},{{3,-3}}}};f.data.box.point=true;f.data.shape.context.mode=2;f.data.shape.context.points={{{0,0,0}},{{1,0,0}},{{0,1,0}},{{0,0,1}}};f.data.ray[9]=1;f.trace.fill(0xa7);std::array<float,3>end{4,5,6};std::memcpy(f.trace.data()+12,end.data(),12);
    for(uint32_t id=0;id<8;++id){const auto p=sourcePhysicsBoxVertexCandidate(f.data.box,id|0x8000);for(unsigned j=0;j<3;++j)require(p[j]==f.data.box.axes[j][(id>>j)&1]);}
    const auto p=sourcePhysicsShapeVertexCandidate(f.data.shape.context,2);require(p==std::array<float,3>{0,1,0});
    f.data.simplex.count=0;for(auto&v:f.data.simplex.vertices){v.point.fill(std::numeric_limits<float>::quiet_NaN());v.featureB=65535;}auto trace=f.trace;contactPlaneInvokeOwned(f,trace);float distance=1;std::memcpy(&distance,trace.data()+36,4);require(distance==0);for(unsigned j=0;j<84;++j)if(j<24||j>=40)require(trace[j]==f.trace[j]);
    f.data.simplex.count=1;f.data.simplex.vertices[0].featureB=1;trace=f.trace;contactPlaneInvokeOwned(f,trace);std::memcpy(&distance,trace.data()+36,4);require(distance==-1);
    f.data.simplex.count=2;f.data.simplex.vertices[0].featureA=0;f.data.simplex.vertices[1].featureA=0x8001;f.data.simplex.vertices[0].featureB=65535;f.data.simplex.vertices[1].featureB=65535;trace=f.trace;contactPlaneInvokeOwned(f,trace);require(trace!=f.trace); // A-edge branch never fetches B.
    for(unsigned variant=0;variant<4;++variant){auto bad=f;bad.data.simplex.count=1;bad.data.simplex.vertices[0].featureB=variant==0?65535:0;if(variant==1)bad.data.ray[9]=std::numeric_limits<float>::quiet_NaN();if(variant==2)bad.data.simplex.count=5;if(variant==3)bad.data.shape.context.points[0][0]=std::numeric_limits<float>::quiet_NaN();auto output=bad.trace;bool rejected=false;try{contactPlaneInvokeOwned(bad,output);}catch(const std::runtime_error&){rejected=true;}require(rejected);require(output==bad.trace);}
    std::cout<<"PASS contact-plane analytical/transaction checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
