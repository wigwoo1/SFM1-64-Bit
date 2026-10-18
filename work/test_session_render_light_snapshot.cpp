#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "film_scene_render_binding_candidate.h"
#include "session_render_light_candidate.h"
#include "projected_light_view_candidate.h"
#include "projected_light_projection_candidate.h"
#include "session_render_light_projection_candidate.h"
#include <iostream>
int main(){try{
 static_assert(sizeof(void*)==8);
 RecoveredMatrix3x4 axis{{{{1,0,0,4}},{{0,1,0,5}},{{0,0,1,6}}}};auto view=projectedLightViewCandidate(axis);
 if(view.clip({4,5,6})!=std::array<float,4>{0,0,0,1}||view.clip({5,5,6})!=std::array<float,4>{0,0,1,1}||view.clip({4,6,6})!=std::array<float,4>{1,0,0,1}||view.clip({4,5,7})!=std::array<float,4>{0,1,0,1})throw std::runtime_error("SDK light view basis");
 auto projection=projectedLightWorldToTextureCandidate(axis,90,90,1,9);
 auto check=[&](std::array<float,3> point,std::array<float,3> expected){auto q=projection.clip(point);for(unsigned i=0;i<3;++i)if(std::abs(q[i]/q[3]-expected[i])>1e-6f)throw std::runtime_error("light unit cube projection");};
 check({5,5,6},{.5f,.5f,0});check({13,5,6},{.5f,.5f,1});check({5,6,7},{0,0,0});check({5,4,5},{1,1,0});
 size_t invalid=0;for(auto args:std::vector<std::array<float,4>>{{0,90,1,9},{90,180,1,9},{90,90,0,9},{90,90,9,1}}){try{projectedLightPerspectiveCandidate(args[0],args[1],args[2],args[3]);}catch(const std::runtime_error&){++invalid;}}if(invalid!=4)throw std::runtime_error("invalid light projection accepted");
 std::cout<<"SDK-derived light unit-cube near/far/edges and invalid ranges passed\n";
 auto graph=loadGraph("work/short-timeline-session.dmx");auto binding=readFilmSceneRenderBindingCandidate(graph,2470);
 if(binding.lights!=std::vector<uint32_t>{3735,3886,4037})throw std::runtime_error("shot light membership");
 auto id=binding.lights.front();GraphTransformHierarchy initial(graph);auto before=readSessionRenderLightCandidate(graph,id,initial);
 for(auto lightId:binding.lights){auto light=readSessionRenderLightCandidate(graph,lightId,initial);auto matrix=sessionRenderLightProjectionCandidate(light);for(unsigned field:{0u,30u}){float distance;std::memcpy(&distance,&light.fields.floatWords[field],4);std::array<float,3> point{};for(unsigned r=0;r<3;++r)point[r]=light.world[r][3]+distance*light.world[r][0];auto q=matrix.clip(point);if(q[3]<=0||std::abs(q[0]/q[3]-.5f)>1e-4f||std::abs(q[1]/q[3]-.5f)>1e-4f||std::abs(q[2]/q[3]-(field==0?0.0f:1.0f))>1e-4f)throw std::runtime_error("session light near/far mapping");}}
 std::cout<<"three installed-session light near/far center projections passed\n";
 auto setWire=[&](uint32_t element,const char*name,size_t offset,float value){bool found=false;for(auto&a:graph.elements.at(element).attributes)if(graph.strings.at(a.name)==name){if(a.values.size()!=1||offset+4>a.values[0].wire.size())throw std::runtime_error("fixture field shape");std::memcpy(a.values[0].wire.data()+offset,&value,4);found=true;}if(!found)throw std::runtime_error("fixture field missing");};
 float original;std::memcpy(&original,&before.fields.localPositionWords[0],4);
 setWire(before.fields.transformReference,"position",0,original+16);
 setWire(id,"intensity",0,2);
 GraphTransformHierarchy updated(graph);auto after=readSessionRenderLightCandidate(graph,id,updated);
 auto parent=initial.parent(id,true);auto parentWorld=parent==UINT32_MAX?GraphTransformHierarchy::identity():initial.world(parent);
 for(unsigned row=0;row<3;++row){float expected=before.world[row][3]+16*parentWorld[row][0];if(std::abs(after.world[row][3]-expected)>.001f)throw std::runtime_error("inherited translated light");}
 float intensity;std::memcpy(&intensity,&after.fields.floatWords[26],4);if(intensity!=2||before.fields.floatWords[26]==after.fields.floatWords[26])throw std::runtime_error("evaluated intensity snapshot");
 setWire(id,"intensity",0,3);if(after.fields.floatWords[26]!=0x40000000u)throw std::runtime_error("snapshot borrowed graph storage");
 setWire(before.fields.transformReference,"position",0,std::numeric_limits<float>::infinity());bool rejected=false;try{GraphTransformHierarchy bad(graph);readSessionRenderLightCandidate(graph,id,bad);}catch(const std::exception&){rejected=true;}if(!rejected)throw std::runtime_error("nonfinite world accepted");
 std::cout<<"x64 shot light membership, evaluated position/intensity, owned snapshot and nonfinite rejection passed; original session file untouched\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
