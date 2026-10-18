#pragma once
#include <array>
#include <vector>
#include <string>
#include <sstream>
#include <istream>
#include <charconv>
#include <limits>
#include <cmath>
#include <stdexcept>
#include <cstdint>
struct StudioObjCornerCandidate{uint32_t position;uint32_t texture=UINT32_MAX,normal=UINT32_MAX;};
struct StudioObjPolygonCandidate{std::vector<StudioObjCornerCandidate> corners;std::string object,group,material,smoothing;};
struct StudioObjModelCandidate{std::vector<std::array<float,3>> positions,textures,normals;std::vector<StudioObjPolygonCandidate> polygons;std::vector<std::string> materialLibraries;};
inline uint32_t studioObjIndexCandidate(const std::string& text,size_t available){
 int64_t index=0;auto r=std::from_chars(text.data(),text.data()+text.size(),index);
 if(text.empty()||r.ec!=std::errc{}||r.ptr!=text.data()+text.size()||index==0)throw std::runtime_error("invalid OBJ index");
 if(index<0){if(index < -static_cast<int64_t>(available))throw std::runtime_error("OBJ relative index range");index+=static_cast<int64_t>(available);}
 else --index;
 if(index<0||static_cast<uint64_t>(index)>=UINT32_MAX)throw std::runtime_error("OBJ index range");return static_cast<uint32_t>(index);
}
inline StudioObjModelCandidate studioReadObjCandidate(std::istream& input){
 StudioObjModelCandidate result;std::string line,object,group,material,smoothing="off";size_t lineNumber=0;
 while(std::getline(input,line)){++lineNumber;try{
  if(line.size()>1048576)throw std::runtime_error("OBJ line too long");auto comment=line.find('#');if(comment!=std::string::npos)line.resize(comment);std::istringstream words(line);std::string command;if(!(words>>command))continue;
  auto rest=[&]{std::string s;std::getline(words>>std::ws,s);return s;};
  if(command=="v"||command=="vt"||command=="vn"){
   std::vector<float> values;std::string numeric;while(words>>numeric){float f=0;const char* first=numeric.data();if(!numeric.empty()&&numeric[0]=='+')++first;auto parsed=std::from_chars(first,numeric.data()+numeric.size(),f);if(parsed.ec!=std::errc{}||parsed.ptr!=numeric.data()+numeric.size()||!std::isfinite(f))throw std::runtime_error("invalid/nonfinite OBJ value");values.push_back(f);}
   std::array<float,3> value{};
   if(command=="v"){if(values.size()!=3&&values.size()!=4)throw std::runtime_error("OBJ position arity");float w=values.size()==4?values[3]:1.0f;if(w==0)throw std::runtime_error("OBJ zero homogeneous coordinate");for(unsigned i=0;i<3;++i){value[i]=values[i]/w;if(!std::isfinite(value[i]))throw std::runtime_error("OBJ position overflow");}result.positions.push_back(value);}
   else if(command=="vn"){if(values.size()!=3)throw std::runtime_error("OBJ normal arity");for(unsigned i=0;i<3;++i)value[i]=values[i];result.normals.push_back(value);}
   else{if(values.empty()||values.size()>3)throw std::runtime_error("OBJ texture arity");for(size_t i=0;i<values.size();++i)value[i]=values[i];result.textures.push_back(value);}
  }else if(command=="f"){
   StudioObjPolygonCandidate face;face.object=object;face.group=group;face.material=material;face.smoothing=smoothing;std::string token;
   while(words>>token){auto slash=token.find('/');auto second=slash==std::string::npos?slash:token.find('/',slash+1);if(second!=std::string::npos&&token.find('/',second+1)!=std::string::npos)throw std::runtime_error("OBJ corner arity");
    StudioObjCornerCandidate c{studioObjIndexCandidate(token.substr(0,slash),result.positions.size())};
    if(slash!=std::string::npos){auto uv=token.substr(slash+1,second==std::string::npos?std::string::npos:second-slash-1);if(!uv.empty())c.texture=studioObjIndexCandidate(uv,result.textures.size());else if(second==std::string::npos)throw std::runtime_error("empty OBJ texture index");if(second!=std::string::npos)c.normal=studioObjIndexCandidate(token.substr(second+1),result.normals.size());}
    face.corners.push_back(c);
   }if(face.corners.size()<3)throw std::runtime_error("OBJ face requires three corners");result.polygons.push_back(std::move(face));
  }else if(command=="o")object=rest();else if(command=="g")group=rest();else if(command=="usemtl")material=rest();else if(command=="s")smoothing=rest();else if(command=="mtllib"){std::string name;while(words>>name)result.materialLibraries.push_back(name);}else throw std::runtime_error("unsupported OBJ record: "+command);
 }catch(const std::exception& e){throw std::runtime_error("OBJ line "+std::to_string(lineNumber)+": "+e.what());}}
 if(input.bad())throw std::runtime_error("OBJ input read failed");
 for(const auto& face:result.polygons)for(const auto& c:face.corners)if(c.position>=result.positions.size()||(c.texture!=UINT32_MAX&&c.texture>=result.textures.size())||(c.normal!=UINT32_MAX&&c.normal>=result.normals.size()))throw std::runtime_error("OBJ final index range");
 if(result.positions.empty()||result.polygons.empty())throw std::runtime_error("OBJ mesh is empty");return result;
}

