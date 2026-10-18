#pragma once
#include <cmath>
// Requires the reconstructed Graph types. Projection conventions are separate.
struct SessionCameraSettingsCandidate {uint32_t transform;float fieldOfView,nearPlane,farPlane;bool orthographic;};
inline SessionCameraSettingsCandidate readSessionCameraSettingsCandidate(const Graph&graph,uint32_t id){
 const auto&camera=graph.elements.at(id);if(graph.strings.at(camera.type)!="DmeCamera")throw std::runtime_error("camera element required");
 auto attribute=[&](const Element&e,const char*name,uint8_t type)->const Attribute&{const Attribute*found=nullptr;for(auto&a:e.attributes)if(graph.strings.at(a.name)==name){if(found)throw std::runtime_error("duplicate camera attribute");found=&a;}if(!found||found->type!=type||found->values.size()!=1)throw std::runtime_error("camera attribute schema");return *found;};
 auto number=[&](const char*name){float value=wireFloat(attribute(camera,name,3).values[0]);if(!std::isfinite(value))throw std::runtime_error("nonfinite camera setting");return value;};
 auto transform=attribute(camera,"transform",1).values[0].reference;if(transform>=graph.elements.size()||graph.strings.at(graph.elements[transform].type)!="DmeTransform")throw std::runtime_error("camera transform reference");
 auto&flag=attribute(camera,"ortho",4).values[0].wire;if(flag.size()!=1||(flag[0]!=0&&flag[0]!=1))throw std::runtime_error("camera ortho flag");SessionCameraSettingsCandidate result{transform,number("fieldOfView"),number("znear"),number("zfar"),flag[0]!=0};if(result.fieldOfView<=0||result.fieldOfView>=180||result.nearPlane<=0||result.farPlane<=result.nearPlane)throw std::runtime_error("camera frustum settings");return result;
}
