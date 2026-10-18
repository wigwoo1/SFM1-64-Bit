#include "geometry_deformation_worker_pool.h"
#include <iostream>
int main(){try{
 std::vector<GeometryMatrixCandidate>palette{{1,0,0,0,0,1,0,0,0,0,1,0}};
 VvdGeometryCandidate good;good.vertices.resize(17);for(auto&v:good.vertices){float weight=1;std::memcpy(v.data(),&weight,4);v[15]=1;}
 good.tangents.resize(17);for(size_t i=0;i<17;++i){std::array<float,3>position{float(i),2,3},normal{1,2,3};std::array<float,4>tangent{3,2,1,-1};std::memcpy(good.vertices[i].data()+16,position.data(),12);std::memcpy(good.vertices[i].data()+28,normal.data(),12);std::memcpy(good.tangents[i].data(),tangent.data(),16);}
 RenderCameraCandidate placement{{0,-2,0,4,3,0,0,-5,0,0,1,6,0,0,0,1}};
 auto expected=deformGeometryBatchCandidate(good,palette,0);for(auto&v:expected){auto p=placement.clip(v.position);v.position={p[0],p[1],p[2]};auto normal=v.normal;auto tangent=v.tangent;for(size_t row=0;row<3;++row){v.normal[row]=placement.rows[row*4]*normal[0]+placement.rows[row*4+1]*normal[1]+placement.rows[row*4+2]*normal[2];v.tangent[row]=placement.rows[row*4]*tangent[0]+placement.rows[row*4+1]*tangent[1]+placement.rows[row*4+2]*tangent[2];}}
 for(size_t count:{size_t(1),size_t(2),size_t(4),size_t(8),size_t(64)}){
  GeometryDeformationWorkerPoolCandidate pool(count);
  auto actual=pool.deform(good,palette,&placement);for(size_t i=0;i<17;++i)if(actual[i].position!=expected[i].position||actual[i].normal!=expected[i].normal||actual[i].uv!=expected[i].uv||actual[i].tangent!=expected[i].tangent||actual[i].hasTangent!=expected[i].hasTangent)throw std::runtime_error("parallel placement changed vertex");
  auto invalid=placement;invalid.rows[15]=2;bool invalidRejected=false;try{pool.deform(good,palette,&invalid);}catch(const std::runtime_error&){invalidRejected=true;}if(!invalidRejected)throw std::runtime_error("nonaffine placement accepted");
  for(size_t size:{size_t(2),size_t(17)}){auto bad=good;bad.vertices.resize(size);bad.tangents.resize(size);bad.vertices[0][15]=4;bad.vertices.back()[12]=255;
   for(int repeat=0;repeat<20;++repeat){bool rejected=false;try{pool.deform(bad,palette);}catch(const std::runtime_error&e){if(std::string(e.what())!="skin influence count")throw;rejected=true;}if(!rejected)throw std::runtime_error("invalid batch accepted");if(pool.deform(good,palette).size()!=17)throw std::runtime_error("pool did not recover");}
  }
  if(!pool.deform({},palette).empty())throw std::runtime_error("empty batch failed");
 }
 std::cout<<"geometry error order passed 200 rejected batches/recoveries workers=1,2,4,8,64 vertices=2,17 empty_ranges=covered placement_normals_tangents_exact nonaffine_rejected\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
