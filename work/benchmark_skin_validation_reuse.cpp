#include "model_skinning_geometry.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
static std::vector<uint8_t> readFile(const char*path){std::ifstream file(path,std::ios::binary);if(!file)throw std::runtime_error("missing model fixture");return {(std::istreambuf_iterator<char>(file)),{}};}
// The pre-optimization vertex path, built from unchanged validating routines.
static GeometryDeformedVertexCandidate originalVertex(const std::array<uint8_t,48>&vertex,const std::array<uint8_t,16>*tangent,const std::vector<GeometryMatrixCandidate>&palette){
 GeometryDeformedVertexCandidate result;result.position=skinGeometryPositionCandidate(vertex,palette);
 std::array<float,3>normal;std::memcpy(normal.data(),vertex.data()+28,12);result.normal=skinGeometryDirectionCandidate(vertex,normal,palette);
 std::memcpy(result.uv.data(),vertex.data()+40,8);for(float value:result.uv)if(!std::isfinite(value))throw std::runtime_error("nonfinite UV");
 if(tangent){std::array<float,4>input;std::memcpy(input.data(),tangent->data(),16);for(float value:input)if(!std::isfinite(value))throw std::runtime_error("nonfinite tangent");auto direction=skinGeometryDirectionCandidate(vertex,{input[0],input[1],input[2]},palette);result.tangent={direction[0],direction[1],direction[2],input[3]};result.hasTangent=true;}
 return result;
}
static bool exact(const GeometryDeformedVertexCandidate&a,const GeometryDeformedVertexCandidate&b){return !std::memcmp(a.position.data(),b.position.data(),12)&&!std::memcmp(a.normal.data(),b.normal.data(),12)&&!std::memcmp(a.uv.data(),b.uv.data(),8)&&!std::memcmp(a.tangent.data(),b.tangent.data(),16)&&a.hasTangent==b.hasTangent;}
static double median(std::vector<double>values){std::sort(values.begin(),values.end());return (values.at(values.size()/2-1)+values.at(values.size()/2))*.5;}
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=4)throw std::runtime_error("MDL VVD output required");
 auto mdl=readFile(argv[1]),vvd=readFile(argv[2]);auto bones=readGeometrySkeletonCandidate(mdl);uint32_t checksum;std::memcpy(&checksum,mdl.data()+8,4);auto geometry=readVvdGeometryCandidate(vvd,checksum,0);
 std::vector<GeometryMatrixCandidate>locals;for(const auto&bone:bones)locals.push_back(bone.local);
 auto palette=evaluateGeometryPosePaletteCandidate(bones,locals);auto originalPalette=palette;
 std::vector<GeometryDeformedVertexCandidate>original(geometry.vertices.size()),optimized(original.size());std::vector<double>oldTimes,newTimes;
 for(int iteration=0;iteration<24;++iteration){
  palette=originalPalette;for(auto&matrix:palette)matrix[3]+=float(iteration)*.125f;
  auto batch=[&](bool reuse){auto start=std::chrono::steady_clock::now();auto&output=reuse?optimized:original;
   for(size_t n=0;n<output.size();++n){auto tangent=geometry.tangents.empty()?nullptr:&geometry.tangents[n];output[n]=reuse?skinGeometryVertexCandidate(geometry.vertices[n],tangent,palette):originalVertex(geometry.vertices[n],tangent,palette);}
   return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();};
  double oldMs,newMs;if(iteration%2){newMs=batch(true);oldMs=batch(false);}else{oldMs=batch(false);newMs=batch(true);}
  for(size_t n=0;n<original.size();++n)if(!exact(original[n],optimized[n]))throw std::runtime_error("vertex bits changed");
  if(iteration>=4){oldTimes.push_back(oldMs);newTimes.push_back(newMs);}
 }
 std::ofstream output(argv[3]);if(!output)throw std::runtime_error("output unavailable");
 auto samples=[&](const std::vector<double>&values){output<<'[';for(size_t n=0;n<values.size();++n){if(n)output<<',';output<<values[n];}output<<']';};
 output<<"{\"vertices\":"<<original.size()<<",\"compared_pairs\":24,\"warmup_pairs\":4,\"timed_pairs\":20,\"original_median_ms\":"<<median(oldTimes)<<",\"optimized_median_ms\":"<<median(newTimes)<<",\"original_samples_ms\":";samples(oldTimes);output<<",\"optimized_samples_ms\":";samples(newTimes);output<<",\"scope\":\"Paired single-thread vertex routine benchmark, alternating order and changing translations; full vertex fields bit-exact. Excludes allocation, worker dispatch, scene preparation, GPU and native SFM.\"}\n";
 std::cout<<"PASS paired skin validation reuse vertices="<<original.size()<<" original_median_ms="<<median(oldTimes)<<" optimized_median_ms="<<median(newTimes)<<" timed_pairs=20 bits_exact=1\n";
 return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
