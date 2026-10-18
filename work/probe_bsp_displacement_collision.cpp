#include "source_bsp_displacement_collision_candidate.h"
#include "source_prepared_world_ray_candidate.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
int main(int argc,char**argv){try{
 if(argc!=4&&argc!=6)throw std::runtime_error("map hdr-type mesh-output [queries trace-output] required");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!in||!out)throw std::runtime_error("workspace files");std::vector<uint8_t>b((std::istreambuf_iterator<char>(in)),{});const int32_t hdr=std::stoi(argv[2]);SourceBspDisplacementCollisionCandidate loaded(b,hdr);uint32_t count=uint32_t(loaded.meshes.size());out.write(reinterpret_cast<const char*>(&loaded.selectedFaceLump),4);out.write(reinterpret_cast<const char*>(&count),4);
 for(const auto&m:loaded.meshes){const std::array<uint32_t,3>header{m.power,m.contents,m.runtimeFlags};out.write(reinterpret_cast<const char*>(header.data()),12);out.write(reinterpret_cast<const char*>(m.corners.data()),48);out.write(reinterpret_cast<const char*>(m.baseNormal.data()),12);out.write(reinterpret_cast<const char*>(m.vertices.data()),m.vertices.size()*12);out.write(reinterpret_cast<const char*>(m.triangles.data()),m.triangles.size()*24);out.write(reinterpret_cast<const char*>(m.tree.leafPairs.data()),m.tree.leafPairs.size()*4);for(const auto&n:m.tree.nodes)out.write(reinterpret_cast<const char*>(n.bounds.data()),96);out.write(reinterpret_cast<const char*>(m.tree.tightBounds.data()),24);out.write(reinterpret_cast<const char*>(m.tree.expandedBounds.data()),24);}
 out.flush();if(!out)throw std::runtime_error("mesh output failed");size_t queries=0;
 if(argc==6){std::ifstream q(argv[4],std::ios::binary);std::ofstream traces(argv[5],std::ios::binary);if(!q||!traces)throw std::runtime_error("query files");SourceBspBrushOnlyTraceCandidate world(b,std::vector<uint32_t>(32768));SourcePreparedWorldRayCandidate combined(world,loaded.meshes);std::array<uint32_t,7>query{};
  while(q.read(reinterpret_cast<char*>(query.data()),28)){std::array<float,3>start,end;std::memcpy(start.data(),query.data(),12);std::memcpy(end.data(),query.data()+3,12);SourcePreparedWorldRayStateCandidate s;s.trace.plane={99,98,97,96};combined.trace(start,end,query[6],s);std::array<uint32_t,11>r{};std::memcpy(r.data(),&s.trace.fraction,4);std::memcpy(r.data()+1,&s.trace.leftSolid,4);r[2]=s.trace.startSolid;r[3]=s.trace.allSolid;r[4]=s.trace.contents;std::memcpy(r.data()+5,s.trace.plane.data(),16);r[9]=s.displacementFlags;r[10]=s.displacementHit;traces.write(reinterpret_cast<const char*>(r.data()),44);++queries;}if(!q.eof()||q.gcount()!=0)throw std::runtime_error("query length");traces.flush();if(!traces)throw std::runtime_error("trace output failed");
 }
 std::cout<<"PASS BSP-backed displacement meshes="<<count<<" face-lump="<<loaded.selectedFaceLump<<" queries="<<queries<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
