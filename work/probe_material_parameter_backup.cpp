#include "material_parameter_backup_candidate.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&i){uint32_t v;need(bool(i.read(reinterpret_cast<char*>(&v),4)),"word");return v;}
static void put(std::ostream&o,uint32_t v){o.write(reinterpret_cast<const char*>(&v),4);}
static std::string text(std::istream&i){auto n=read(i);need(n<=4095,"text extent");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text");return s;}
static void text(std::ostream&o,const std::string&s){put(o,uint32_t(s.size()));o.write(s.data(),s.size());}
struct Source{MaterialTypedDispatchCandidate value;MaterialParameterResourceTextCandidate resource;};
static Source source(std::istream&in){
 auto type=read(in),count=read(in),integer=read(in),environment=read(in),metadata=read(in);
 std::array<uint32_t,16>numbers{};for(auto&v:numbers)v=read(in);auto raw=text(in),resource=text(in);
 need(type<=8&&type!=5&&count<=4&&metadata<=1&&environment<=1,"source scope");
 Source result;auto&v=result.value;v.created=true;std::array<uint32_t,4>args{};std::copy_n(numbers.begin(),4,args.begin());
 auto construct=[&](MaterialValueConstructionKindCandidate kind,uint32_t n=0,std::string_view s={}){need(materialValueConstructCandidate(v.value,kind,args,n,s),"owned typed construction");};
 switch(type){
 case 0:construct(MaterialValueConstructionKindCandidate::Float);break;
 case 1:construct(MaterialValueConstructionKindCandidate::String,0,raw);break;
 case 2:construct(MaterialValueConstructionKindCandidate::Vector,std::max(count,1u));if(!count)v.value.numeric.typeAndMetadata&=~0x70u;break;
 case 4:args[0]=integer;construct(MaterialValueConstructionKindCandidate::Integer);break;
 case 6:construct(MaterialValueConstructionKindCandidate::Undefined);break;
 case 7:construct(MaterialValueConstructionKindCandidate::Undefined);need(materialTypedMatrixSetCandidate(v,numbers),"owned matrix setter");break;
 case 3:case 8:construct(MaterialValueConstructionKindCandidate::Undefined);v.value.numeric.typeAndMetadata=(v.value.numeric.typeAndMetadata&~15u)|type;result.resource.name=resource;result.resource.environmentTexture=environment!=0;break;
 default:throw std::runtime_error("source type");
 }
 v.value.numeric.typeAndMetadata=(v.value.numeric.typeAndMetadata&~0x80u)|(metadata?0x80u:0u);return result;
}
struct Trace:MaterialSessionBackupServicesCandidate{
 MaterialParameterBackupServicesCandidate service;struct Event{uint32_t kind,index,count;};std::vector<Event>events;
 Trace(const MaterialParameterCollectionCandidate&values,const std::vector<MaterialParameterResourceTextCandidate>&resources):service(values,[&resources](size_t i,const MaterialTypedDispatchCandidate&){return resources.at(i);}){}
 MaterialSessionOverrideLookupCandidate find(std::string_view name)override{auto result=service.find(name);need(result.found&&result.variable,"case lookup");events.push_back({0,uint32_t(result.variable-1),0});return result;}
 uint32_t integer(uintptr_t p)override{events.push_back({1,uint32_t(p-1),0});return service.integer(p);}
 uint32_t scalar(uintptr_t p)override{events.push_back({2,uint32_t(p-1),0});return service.scalar(p);}
 std::array<uint32_t,4>vector(uintptr_t p,uint32_t n)override{events.push_back({3,uint32_t(p-1),n});return service.vector(p,n);}
 std::array<uint32_t,16>matrix(uintptr_t p)override{events.push_back({4,uint32_t(p-1),16});return service.matrix(p);}
 std::optional<std::string>stringValue(uintptr_t p)override{events.push_back({5,uint32_t(p-1),0});return service.stringValue(p);}
};
int main(int argc,char**argv){try{
 static_assert(sizeof(void*)==8,"native independent x64 composition");need(argc==5,"absent-image sources capture-query fresh-output");need(!std::ifstream(argv[1]).good(),"owned image absent");need(!std::ifstream(argv[4]).good(),"fresh output");
 std::ifstream src(argv[2],std::ios::binary),in(argv[3],std::ios::binary);std::ofstream out(argv[4],std::ios::binary);need(bool(src)&&bool(in)&&bool(out),"IO");
 auto n=read(src);need(n&&n<20000,"source count");std::vector<Source>sources;for(unsigned i=0;i<n;++i)sources.push_back(source(src));need(src.peek()==std::char_traits<char>::eof(),"source tail");
 auto cases=read(in);need(cases&&cases<100000,"capture cases");put(out,cases);unsigned total=0;
 for(unsigned c=0;c<cases;++c){auto flags=read(in);int32_t phase;auto rawPhase=read(in);std::memcpy(&phase,&rawPhase,4);auto count=read(in);need(count<=256,"collection extent");
  MaterialParameterCollectionCandidate parameters;std::vector<MaterialParameterResourceTextCandidate>resources;std::vector<MaterialSessionOverrideInputCandidate>rows;
  for(unsigned i=0;i<count;++i){MaterialSessionOverrideInputCandidate row;row.typeFlags=read(in);auto presence=read(in),nameState=read(in);for(auto&w:row.words)w=read(in);std::array<uint32_t,16>live{};for(auto&w:live)w=read(in);row.name=text(in);row.textureName=text(in);auto ignoredBoundaryText=text(in);(void)ignoredBoundaryText;
   need((row.typeFlags&31u)==5&&presence==3&&nameState==0&&live[15]<sources.size(),"string template/source mapping");
   auto retained=sources[live[15]];retained.value.name=row.name;parameters.values.push_back(std::move(retained.value));resources.push_back(std::move(retained.resource));rows.push_back(std::move(row));
  }
  Trace service(parameters,resources);if((flags&3)==3)need(captureMaterialSessionBackupCandidate(rows,service,phase),"typed capture");
  put(out,c);put(out,count);for(const auto&r:rows){put(out,r.typeFlags);for(auto w:r.words)put(out,w);auto s=*r.textureName;s.resize(s.find('\0')==std::string::npos?s.size():s.find('\0'));text(out,s);}
  put(out,uint32_t(service.events.size()));for(const auto&e:service.events){put(out,e.kind);put(out,e.index);put(out,e.count);}total+=uint32_t(service.events.size());
 }
 need(in.peek()==std::char_traits<char>::eof(),"query tail");out.flush();need(bool(out),"complete output");std::cout<<"PASS composed_backup_cases="<<cases<<" getter_events="<<total<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
