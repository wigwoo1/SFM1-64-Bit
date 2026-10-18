#include "source_phy_compact_geometry_candidate.h"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned AMD64 PHY geometry");if(argc!=3)throw std::runtime_error("PHY input mesh output required");std::ifstream in(argv[1],std::ios::binary);if(!in)throw std::runtime_error("PHY input read");const std::vector<uint8_t>b((std::istreambuf_iterator<char>(in)),{});const auto before=b;const SourcePhyCompactGeometryCandidate geometry(b);if(b!=before)throw std::runtime_error("PHY input mutation");std::ofstream out(argv[2],std::ios::binary);if(!out)throw std::runtime_error("PHY output open");const uint16_t solids=uint16_t(geometry.solids.size());out.write(reinterpret_cast<const char*>(&solids),2);size_t convexes=0,triangles=0;
    for(const auto&solid:geometry.solids){const uint32_t count=uint32_t(solid.size());out.write(reinterpret_cast<const char*>(&count),4);for(const auto&convex:solid){const uint32_t countTriangles=uint32_t(convex.triangles.size());out.write(reinterpret_cast<const char*>(&convex.gameData),4);out.write(reinterpret_cast<const char*>(&countTriangles),4);for(const auto&v:convex.triangles)out.write(reinterpret_cast<const char*>(v.data()),36);++convexes;triangles+=countTriangles;}}
    out.flush();if(!out)throw std::runtime_error("PHY output failure");std::cout<<"PASS owned PHY solids="<<solids<<" convexes="<<convexes<<" triangles="<<triangles<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
