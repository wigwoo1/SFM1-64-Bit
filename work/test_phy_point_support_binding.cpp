#include "source_phy_point_support_binding_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
    unsigned checks=0;auto require=[&](bool ok){if(!ok)throw std::runtime_error("point support binding assertion");++checks;};
    SourcePhyConvexGeometryCandidate c;c.rawLedgeHeader[3]=1;c.rawTriangles={{{0,0,1,2}}};
    for(const auto&p:std::array<std::array<float,4>,3>{{{{0,0,0,0}},{{1,2,3,0}},{{-1,1,-2,0}}}}){std::array<uint32_t,4>bits;std::memcpy(bits.data(),p.data(),16);c.rawPoints.push_back(bits);}
    const std::array<float,9>identity{1,0,0,0,1,0,0,0,1};const auto metadata=sourcePhysicsSupportMetadataCandidate(c,7);const auto before=c;
    const auto packed=sourcePhyPointSupportBindingCandidate(c,7,&metadata,identity);require(packed.mode==2&&packed.points.size()==4&&packed.points[2]==packed.points[3]);require(packed.transform[3]==0&&packed.transform[7]==0&&packed.transform[11]==0);require(packed.points[1][0]==39.370079040527344f&&packed.points[1][1]==3*39.370079040527344f&&packed.points[1][2]==2*(-39.370079040527344f));
    const auto walk=sourcePhyPointSupportBindingCandidate(c,7,nullptr,identity);require(walk.mode==0&&!walk.seeded&&walk.points.size()==3&&walk.triangles==c.rawTriangles);require(walk.points[1]==std::array<float,3>{1,2,3});
    auto seeded=metadata;seeded.payload={};seeded.payload[2]=1;uint16_t seed=2;std::memcpy(seeded.payload.data()+4,&seed,2);const auto seedContext=sourcePhyPointSupportBindingCandidate(c,7,&seeded,identity);require(seedContext.mode==0&&seedContext.seeded&&seedContext.seeds[0]==2);
    auto rejected=[&](const SourcePhyConvexGeometryCandidate&shape,uint32_t id,const SourcePhysicsSupportMetadataRecordCandidate*meta,const std::array<float,9>&basis){bool failed=false;try{sourcePhyPointSupportBindingCandidate(shape,id,meta,basis);}catch(const std::runtime_error&){failed=true;}require(failed);};
    rejected(c,6,&metadata,identity);auto bad=c;bad.rawTriangles.clear();rejected(bad,7,&metadata,identity);bad=c;bad.rawPoints.clear();rejected(bad,7,&metadata,identity);auto basis=identity;basis[0]=std::numeric_limits<float>::quiet_NaN();rejected(c,7,&metadata,basis);basis=identity;basis[0]=std::numeric_limits<float>::max();rejected(c,7,&metadata,basis);
    auto invalid=metadata;invalid.payload[2]=4;invalid.payload[3]=15;rejected(c,7,&invalid,identity);invalid=metadata;invalid.payload[0]=127;rejected(c,7,&invalid,identity);bad=c;bad.rawPoints[1][0]=0x7fc00000;rejected(bad,7,&metadata,identity);
    require(c.rawPoints==before.rawPoints&&c.rawTriangles==before.rawTriangles&&metadata.ledge==7);std::cout<<"PASS point support binding analytical/guard checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
