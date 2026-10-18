#include "source_physics_support_metadata_candidate.h"
#include <iostream>
#include <limits>
int main(){try{
    unsigned checks=0;auto require=[&](bool pass){if(!pass)throw std::runtime_error("support metadata assertion");++checks;};SourcePhyConvexGeometryCandidate c;c.rawLedgeHeader[3]=1;c.rawTriangles={{{0,0,1,2}}};std::array<uint8_t,20>initial;initial.fill(0xa7);
    auto result=sourcePhysicsSupportMetadataCandidate(c,0xabcdef01,initial);uint16_t count=0,first=0;std::memcpy(&count,result.payload.data(),2);std::memcpy(&first,result.payload.data()+4,2);require(result.ledge==0xabcdef01&&count==3&&first==0&&result.payload[2]==2&&result.payload[3]==0);for(size_t i=6;i<20;++i)require(result.payload[i]==0xa7);
    c.rawTriangles[0]={0,0,2,4};result=sourcePhysicsSupportMetadataCandidate(c,7,initial);std::memcpy(&count,result.payload.data(),2);require(count==3&&result.payload[2]==4&&result.payload[3]==5);for(size_t i=6;i<11;++i)require(result.payload[i]==1);for(size_t i=11;i<20;++i)require(result.payload[i]==0xa7);
    c.rawTriangles[0]={0,0,1,126};result=sourcePhysicsSupportMetadataCandidate(c,1,initial);require(result.payload[2]==4&&result.payload[3]==3&&result.payload[6]==2&&result.payload[7]==124&&result.payload[8]==1);
    c.rawLedgeHeader[3]=4;c.rawTriangles={{{0,0x00060000,0x000d0001,0x00060002}},{{0,0x00060000,0x00070003,0x7ffa0001}},{{0,0x7ffa0000,0x00040002,0x7ffa0003}},{{0,0x7ff90001,0x7ffc0003,0x7ff30002}}};
    const std::array<std::array<float,4>,4>points{{{{1,1,1,0}},{{-1,-1,1,0}},{{-1,1,-1,0}},{{1,-1,-1,0}}}};for(const auto&p:points){std::array<uint32_t,4>bits;std::memcpy(bits.data(),p.data(),16);c.rawPoints.push_back(bits);}const auto original=c;
    c.rawLedgeHeader[2]=1;result=sourcePhysicsSupportMetadataCandidate(c,0,initial);std::memcpy(&count,result.payload.data(),2);uint16_t positive=0,negative=0;std::memcpy(&positive,result.payload.data()+4,2);std::memcpy(&negative,result.payload.data()+18,2);require(result.payload[2]==1&&result.payload[3]==0&&count==0&&positive==0&&negative==2);
    require(sourcePhysicsSupportMetadataTableCandidate({}).empty());std::vector<SourcePhyConvexGeometryCandidate>solid(32,original);const auto table=sourcePhysicsSupportMetadataTableCandidate(solid);require(table.size()==32&&table.back().ledge==31&&table.back().payload[2]==2);solid.resize(33);solid[0].rawTriangles.clear();require(sourcePhysicsSupportMetadataTableCandidate(solid).empty());
    auto bad=c;bad.rawPoints.clear();bool rejected=false;try{sourcePhysicsSupportMetadataCandidate(bad,0,initial);}catch(const std::runtime_error&){rejected=true;}require(rejected);bad=c;bad.rawPoints[0][0]=0x7fc00000;rejected=false;try{sourcePhysicsSupportMetadataCandidate(bad,0,initial);}catch(const std::runtime_error&){rejected=true;}require(rejected);require(initial[0]==0xa7&&initial[19]==0xa7);
    std::cout<<"PASS support metadata analytical/boundary checks="<<checks<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
