#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "clip_hierarchy.h"
#include "channel_binding.h"
#include "session_quaternion_runtime.h"
#include "float_curveinfo_schema.h"
int main(int argc,char**argv){try{if(argc!=3)return 1;auto g=loadGraph(argv[1]);uint32_t channel=0xffffffff;
for(uint32_t id=0;id<g.elements.size();++id)if(g.strings.at(g.elements[id].type)=="DmeChannel"){ChannelBinding b(g,id);if(b.destination.present()&&b.destination.scalarType==13&&b.log!=0xffffffff&&b.layers.size()==1){channel=id;break;}}
if(channel==0xffffffff)throw std::runtime_error("no vector channel");ChannelBinding initial(g,channel);auto log=initial.log,layer=initial.layers[0];
auto intern=[&](const std::string&s){auto it=std::find(g.strings.begin(),g.strings.end(),s);if(it!=g.strings.end())return static_cast<uint32_t>(it-g.strings.begin());g.strings.push_back(s);return static_cast<uint32_t>(g.strings.size()-1);};
Element info;info.id.fill(char(0x72));for(const auto&e:g.elements)if(e.id==info.id)throw std::runtime_error("test UUID collision");info.type=intern("DmeQuaternionCurveInfo");info.name=intern("vector curve integration fixture");
for(const auto&schema:floatCurveInfoSchemaRecovered){Attribute a;a.name=intern(schema.name);a.type=schema.type;std::string name=schema.name;if(name=="leftEdgeValue"||name=="rightEdgeValue"||name=="defaultEdgeZeroValue")a.type=13;Value v;v.wire.assign(a.type==13?16:a.type==4?1:4,0);if(name=="defaultCurveType")setWireU32(v,6*257);a.values.push_back(v);info.attributes.push_back(a);}
auto infoIndex=static_cast<uint32_t>(g.elements.size());g.elements.push_back(info);runtimeAttribute(g,log,"curveinfo")->values[0].reference=infoIndex;
auto*t=runtimeAttribute(g,layer,"times"),*v=runtimeAttribute(g,layer,"values"),*k=runtimeAttribute(g,layer,"curvetypes");t->values.clear();v->values.clear();k->values.clear();
for(unsigned i=0;i<4;++i){Value tv;tv.wire.resize(4);setWireU32(tv,i*10000);t->values.push_back(tv);Value vv;vv.wire.resize(16);v->values.push_back(vv);for(unsigned c=0;c<4;++c)setFloat(*v,i,c==3?1.f:0.f,c);Value kv;kv.wire.resize(4);setWireU32(kv,6*257);k->values.push_back(kv);}
setWireU32(runtimeAttribute(g,channel,"mode")->values[0],3);std::array<float,3>edge{};unsigned checks=0;
auto verify=[&](Graph&graph){ChannelBinding b(graph,channel);for(auto tick:{5000,15000,25000}){auto*a=runtimeAttribute(graph,b.destination.element,b.destination.attribute);for(unsigned c=0;c<4;++c)setFloat(*a,b.destination.index,12345.f,c);SessionAttributeChanges changes;if(!executeSessionQuaternionPlay(graph,b,tick,&changes)||changes.attributes.size()!=1)throw std::runtime_error("curve play rejected");for(unsigned c=0;c<4;++c)if(wireFloat(a->values[b.destination.index],c)!=(c==3?1.f:0.f))throw std::runtime_error("curve destination mismatch");++checks;}};
verify(g);saveGraph(g,argv[2]);auto reload=loadGraph(argv[2]);verify(reload);std::cout<<"quaternion_curve_channel_checks="<<checks<<" save_reload=verified\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
