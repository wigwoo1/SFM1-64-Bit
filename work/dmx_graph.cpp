// Native graph reconstruction. Fixed-width values retain exact wire bytes.
#define main table_scanner_main
#include "dmx_tables.cpp"
#undef main
#include <array>
#include <memory>
#include <filesystem>
#include <cstring>
struct Value {std::vector<char> wire;std::string text;uint32_t reference=0xffffffff;uint32_t stringIndex=0xffffffff;};
struct Attribute {uint32_t name;uint8_t type;std::vector<Value> values;};
struct Element {uint32_t type,name;std::array<char,16> id;std::vector<Attribute> attributes;};
struct Graph {std::string header;std::vector<std::string> strings;std::vector<Element> elements;};
uint32_t wireU32(const Value&v,size_t component=0){size_t o=component*4;if(o>v.wire.size()||v.wire.size()-o<4)throw std::runtime_error("missing numeric component");uint32_t bits=0;for(unsigned i=0;i<4;++i)bits|=uint32_t(static_cast<unsigned char>(v.wire[o+i]))<<(8*i);return bits;}
int32_t wireInt(const Value&v){auto bits=wireU32(v);int32_t n;std::memcpy(&n,&bits,4);return n;}
float wireFloat(const Value&v,size_t component=0){auto bits=wireU32(v,component);float n;std::memcpy(&n,&bits,4);return n;}
void setWireU32(Value&v,uint32_t bits,size_t component=0){size_t o=component*4;if(o>v.wire.size()||v.wire.size()-o<4)throw std::runtime_error("missing numeric component");for(unsigned i=0;i<4;++i)v.wire[o+i]=static_cast<char>(bits>>(8*i));}
void setFloat(Attribute&a,size_t value,float n,size_t component=0){unsigned type=a.type>=15?a.type-14:a.type;if(type!=3&&(type<9||type>14))throw std::runtime_error("not float typed");uint32_t bits;std::memcpy(&bits,&n,4);setWireU32(a.values.at(value),bits,component);}
Graph loadGraph(const char* path){
 Reader r(path);auto h=r.cstr();if(h!="<!-- dmx encoding binary 5 format sfm_session 20 -->\n"&&h!="<!-- dmx encoding binary 5 format sfm_session 20 -->")throw std::runtime_error("unsupported header");
 Graph g;g.header=h;auto ns=r.count();g.strings.reserve(ns);for(uint32_t i=0;i<ns;++i)g.strings.push_back(r.cstr());
 auto ne=r.count();g.elements.reserve(ne);
 auto index=[&](){auto n=r.u32();if(n>=ns)throw std::runtime_error("invalid string index");return n;};
 for(uint32_t i=0;i<ne;++i){Element e;e.type=index();e.name=index();r.bytes(e.id.data(),16);g.elements.push_back(std::move(e));}
 for(auto&e:g.elements){auto na=r.count();e.attributes.reserve(na);for(uint32_t j=0;j<na;++j){
  Attribute a;a.name=index();a.type=r.u8();bool array=a.type>=15;unsigned scalar=array?a.type-14:a.type;
  if(scalar<1||scalar>14)throw std::runtime_error("unsupported type");auto n=array?r.count():1;a.values.reserve(n);
  for(uint32_t k=0;k<n;++k){Value v;
   if(scalar==1){v.reference=r.u32();if(v.reference==0xfffffffe)v.text=r.cstr();else if(v.reference!=0xffffffff&&v.reference>=ne)throw std::runtime_error("bad reference");}
   else if(scalar==5){if(array)v.text=r.cstr();else {v.stringIndex=index();v.text=g.strings[v.stringIndex];}}
   else {static const unsigned widths[]={0,0,4,4,1,0,0,4,4,8,12,16,12,16,64};auto bytes=scalar==6?r.blobCount():widths[scalar];v.wire.resize(bytes);if(bytes)r.bytes(v.wire.data(),bytes);}
   a.values.push_back(std::move(v));
  }e.attributes.push_back(std::move(a));
 }}
 if(static_cast<uint64_t>(r.f.tellg())!=r.size)throw std::runtime_error("trailing data");return g;
}
uint32_t internString(Graph&g,const std::string&s){
 if(s.find('\0')!=std::string::npos)throw std::runtime_error("embedded NUL string");
 for(size_t i=0;i<g.strings.size();++i)if(g.strings[i]==s)return static_cast<uint32_t>(i);
 if(g.strings.size()>=UINT32_MAX)throw std::runtime_error("string table full");g.strings.push_back(s);return static_cast<uint32_t>(g.strings.size()-1);
}
void setString(Graph&g,Attribute&a,size_t value,const std::string&s){
 if(a.type!=5&&a.type!=19)throw std::runtime_error("attribute is not string typed");
 auto&v=a.values.at(value);auto index=internString(g,s);v.text=s;v.stringIndex=index;
}
void validateGraph(const Graph&g){
 auto index=[&](uint32_t i){if(i>=g.strings.size())throw std::runtime_error("invalid graph string index");};
 for(auto&s:g.strings)if(s.find('\0')!=std::string::npos)throw std::runtime_error("embedded NUL string");
 for(auto&e:g.elements){index(e.type);index(e.name);for(auto&a:e.attributes){index(a.name);bool array=a.type>=15;unsigned scalar=array?a.type-14:a.type;
  if(scalar<1||scalar>14||(!array&&a.values.size()!=1))throw std::runtime_error("invalid graph type/cardinality");
  for(auto&v:a.values){if(scalar==1){if(v.reference!=0xffffffff&&v.reference!=0xfffffffe&&v.reference>=g.elements.size())throw std::runtime_error("invalid graph reference");if(v.reference==0xfffffffe&&v.text.find('\0')!=std::string::npos)throw std::runtime_error("invalid external reference");}
   else if(scalar==5){if(v.text.find('\0')!=std::string::npos)throw std::runtime_error("invalid string value");if(!array){index(v.stringIndex);if(g.strings[v.stringIndex]!=v.text)throw std::runtime_error("inconsistent string value");}}
   else if(scalar!=6){static const unsigned widths[]={0,0,4,4,1,0,0,4,4,8,12,16,12,16,64};if(v.wire.size()!=widths[scalar])throw std::runtime_error("invalid wire value size");}
  }
 }}
}
void saveGraph(const Graph&g,const char*path){
 validateGraph(g);
 if(std::filesystem::exists(path))throw std::runtime_error("output already exists; refusing overwrite");
 std::ofstream f(path,std::ios::binary);if(!f)throw std::runtime_error("output open failed");
 auto bytes=[&](const char*p,size_t n){f.write(p,n);if(!f)throw std::runtime_error("output write failed");};
 auto u32=[&](uint32_t v){char b[4];for(unsigned i=0;i<4;++i)b[i]=static_cast<char>(v>>(8*i));bytes(b,4);};
 auto str=[&](const std::string&s){bytes(s.c_str(),s.size()+1);};
 auto count=[&](size_t n){if(n>UINT32_MAX)throw std::runtime_error("wire count overflow");u32(static_cast<uint32_t>(n));};
 str(g.header);count(g.strings.size());for(auto&s:g.strings)str(s);count(g.elements.size());
 for(auto&e:g.elements){u32(e.type);u32(e.name);bytes(e.id.data(),16);}
 for(auto&e:g.elements){count(e.attributes.size());for(auto&a:e.attributes){u32(a.name);char tag=static_cast<char>(a.type);bytes(&tag,1);bool array=a.type>=15;unsigned scalar=array?a.type-14:a.type;if(array)count(a.values.size());
  for(auto&v:a.values){if(scalar==1){u32(v.reference);if(v.reference==0xfffffffe)str(v.text);}else if(scalar==5){if(array)str(v.text);else u32(v.stringIndex);}else {if(scalar==6)count(v.wire.size());if(!v.wire.empty())bytes(v.wire.data(),v.wire.size());}}
 }}f.flush();if(!f)throw std::runtime_error("output flush failed");
}
#ifndef DMX_GRAPH_NO_MAIN
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"x64 required");if(argc!=2&&argc!=3)throw std::runtime_error("usage: dmx_graph session.dmx [new-output.dmx]");
 auto g=loadGraph(argv[1]);uint64_t count=0;bool active=false;uint32_t activeIndex=0xffffffff;
 for(auto&e:g.elements)count+=e.attributes.size();
 if(!g.elements.empty())for(auto&a:g.elements[0].attributes)if(g.strings[a.name]=="activeClip"&&a.type==1&&!a.values.empty()){activeIndex=a.values[0].reference;active=activeIndex<g.elements.size();}
 if(argc==3)saveGraph(g,argv[2]);
 std::cout<<"{\"pointer_bits\":64,\"elements\":"<<g.elements.size()<<",\"attributes\":"<<count<<",\"active_clip_resolved\":"<<(active?"true":"false")<<",\"active_clip_index\":"<<activeIndex<<",\"scope\":\"retained_wire_graph\"}\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
#endif
