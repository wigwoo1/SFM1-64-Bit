#include "source_physics_support_metadata_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned AMD64 metadata binding");if(argc!=3)throw std::runtime_error("PHY input metadata output required");std::ifstream in(argv[1],std::ios::binary);if(!in)throw std::runtime_error("owned metadata PHY read");const std::vector<uint8_t>b((std::istreambuf_iterator<char>(in)),{});const auto before=b;SourcePhyCompactGeometryCandidate geometry(b,true);std::ofstream out(argv[2],std::ios::binary);if(!out)throw std::runtime_error("owned metadata output open");const uint16_t solids=uint16_t(geometry.solids.size());out.write(reinterpret_cast<const char*>(&solids),2);size_t total=0;
    for(const auto&solid:geometry.solids){const uint32_t convexes=uint32_t(solid.size());const auto records=sourcePhysicsSupportMetadataTableCandidate(solid);const uint32_t count=uint32_t(records.size());out.write(reinterpret_cast<const char*>(&convexes),4);out.write(reinterpret_cast<const char*>(&count),4);for(const auto&r:records){out.write(reinterpret_cast<const char*>(&r.ledge),4);out.write(reinterpret_cast<const char*>(r.payload.data()),20);++total;}}
    if(b!=before)throw std::runtime_error("owned metadata PHY changed");out.flush();if(!out)throw std::runtime_error("owned metadata output write");std::cout<<"PASS owned PHY support metadata records="<<total<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
