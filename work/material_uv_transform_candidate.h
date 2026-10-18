#pragma once
#include "vmt_keyvalues_candidate.h"
#include <array>
#include <cmath>
#include <sstream>

using MaterialUvTransformCandidate=std::array<float,8>; // row0.xy+offset, row1.xy+offset
inline MaterialUvTransformCandidate identityMaterialUvTransformCandidate(){return {1,0,0,0,0,1,0,0};}
inline MaterialUvTransformCandidate materialUvTransformCandidate(const VmtNodeCandidate&vmt,const char*parameter="$basetexturetransform"){
 const VmtNodeCandidate*node=nullptr;for(const auto&child:vmt.children)if(vmtAsciiLowerCandidate(child.key)==parameter){if(node||child.block)throw std::runtime_error("ambiguous texture transform");node=&child;}
 if(!node)return identityMaterialUvTransformCandidate();std::string text=node->value;
 if(!text.empty()&&text.front()=='['){std::istringstream input(text.substr(1));std::array<float,16>m{};for(float&value:m)if(!(input>>value)||!std::isfinite(value))throw std::runtime_error("texture transform matrix encoding");char close=0;if(!(input>>close)||close!=']')throw std::runtime_error("texture transform matrix terminator");std::string extra;if(input>>extra)throw std::runtime_error("texture transform matrix trailing value");if(std::abs(m[2])>1e-6f||std::abs(m[6])>1e-6f||std::abs(m[8])>1e-6f||std::abs(m[9])>1e-6f||std::abs(m[10]-1)>1e-6f||std::abs(m[11])>1e-6f||std::abs(m[12])>1e-6f||std::abs(m[13])>1e-6f||std::abs(m[14])>1e-6f||std::abs(m[15]-1)>1e-6f)throw std::runtime_error("non-2D base texture transform");return {m[0],m[1],m[3],0,m[4],m[5],m[7],0};}
 std::istringstream input(text);std::array<float,2>center{.5f,.5f},scale{1,1},translate{0,0};float degrees=0;std::string key;
 while(input>>key){for(char&c:key)if(c>='A'&&c<='Z')c=char(c-'A'+'a');if(key!="center"&&key!="scale"&&key!="rotate"&&key!="translate")throw std::runtime_error("unsupported VMT texture transform token: "+key);std::string first;if(!(input>>first))throw std::runtime_error("incomplete VMT texture transform");size_t consumed=0;float x=std::stof(first,&consumed);if(consumed!=first.size()||!std::isfinite(x))throw std::runtime_error("invalid VMT texture transform number");if(key=="rotate"){degrees=x;continue;}float y=x;auto position=input.tellg();std::string second;if(input>>second){try{size_t secondConsumed=0;y=std::stof(second,&secondConsumed);if(secondConsumed!=second.size()||!std::isfinite(y))throw std::runtime_error("invalid VMT texture transform number");}catch(const std::invalid_argument&){input.clear();input.seekg(position);}catch(const std::out_of_range&){throw std::runtime_error("VMT texture transform number range");}}if(key=="center")center={x,y};else if(key=="scale")scale={x,y};else translate={x,y};}
 constexpr float pi=3.14159265358979323846f;float angle=degrees*pi/180.f,c=std::cos(angle),s=std::sin(angle);float a=c*scale[0],b=-s*scale[1],d=s*scale[0],e=c*scale[1];float tx=translate[0]+center[0]-a*center[0]-b*center[1],ty=translate[1]+center[1]-d*center[0]-e*center[1];MaterialUvTransformCandidate result{a,b,tx,0,d,e,ty,0};for(float value:result)if(!std::isfinite(value))throw std::runtime_error("VMT texture transform overflow");return result;
}
