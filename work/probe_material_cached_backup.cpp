#ifdef SFM64_NATIVE_CACHED_BACKUP
#define SFM64_NATIVE_GET_STRING
#define main preservedGetStringMain
#include "probe_material_get_string.cpp"
#undef main
#else
#define main preservedStringCompositionMain
#include "probe_material_parameter_backup.cpp"
#undef main
#endif

// Original and owned getter outputs have the same semantic record. The six
// cache words are observations BEFORE reading; matrix backing is not retyped.
static int getters(const char*image,const char*query,const char*destination){
 need(!std::ifstream(destination).good(),"fresh getter output");std::ifstream in(query,std::ios::binary);std::ofstream out(destination,std::ios::binary);need(bool(in)&&bool(out),"getter IO");
#ifdef SFM64_NATIVE_CACHED_BACKUP
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);AddVectoredExceptionHandler(1,&fault);GetFixture fixture(image);unsigned control=0,previous=0;_controlfp_s(&previous,0,0);_controlfp_s(&control,_RC_NEAR|_PC_53,_MCW_RC|_MCW_PC);unsigned totalAlloc=0,totalFree=0;
#else
 need(!std::ifstream(image).good(),"owned image absent");
#endif
 const auto oldMx=_mm_getcsr();_mm_setcsr(0x1f80);auto cases=read(in);need(cases&&cases<20000,"source extent");put(out,cases);
 for(unsigned ci=0;ci<cases;++ci){
  std::array<uint32_t,6>cache{};uint32_t integer=0,scalar=0;std::array<std::array<uint32_t,4>,3>vectors{};std::array<uint32_t,16>matrix{};
#ifdef SFM64_NATIVE_CACHED_BACKUP
  const auto type=read(in),components=read(in),bits=read(in),environment=read(in),metadata=read(in);std::array<uint32_t,16>numbers{};for(auto&w:numbers)w=read(in);auto raw=text(in),resource=text(in);(void)resource;
  need((type<=2||type==4||type==6||type==7)&&components<=4&&!environment&&metadata<=1,"cached source domain");fixture.begin(true);std::array<uint8_t,40>object{};
  if(type==0||type==4){using Ctor=void*(__thiscall*)(void*,void*,const char*,uint32_t);reinterpret_cast<Ctor>(type==0?0x10017910:0x100179b0)(object.data(),nullptr,nullptr,type==0?numbers[0]:bits);}
  else if(type==1){using Ctor=void*(__thiscall*)(void*,void*,const char*,const char*);reinterpret_cast<Ctor>(0x10017780)(object.data(),nullptr,nullptr,raw.c_str());}
  else if(type==2){using Ctor=void*(__thiscall*)(void*,void*,const char*,const void*,int);reinterpret_cast<Ctor>(0x10017850)(object.data(),nullptr,nullptr,numbers.data(),int(std::max(components,1u)));if(!components)object[28]=2;}
  else{using Ctor=void*(__thiscall*)(void*,void*,const char*);reinterpret_cast<Ctor>(0x10017a50)(object.data(),nullptr,nullptr);if(type==7){using Set=void(__thiscall*)(void*,const void*);reinterpret_cast<Set>(0x10019f70)(object.data(),numbers.data());}}
  object[28]|=metadata?0x80:0;const auto before=object;std::memcpy(&cache[0],object.data()+28,4);cache[0]&=0xffffu;std::memcpy(&cache[1],object.data()+8,20);fixture.queues=0;
  using Int=uint32_t(__thiscall*)(void*);integer=reinterpret_cast<Int>(0x10017bd0)(object.data());
  using Float=float(__thiscall*)(void*);float f=reinterpret_cast<Float>(0x10017c20)(object.data());std::memcpy(&scalar,&f,4);
  using Vec=void(__thiscall*)(void*,uint32_t*,int);
  for(unsigned i=0;i<3;++i){std::array<uint32_t,7>guard;guard.fill(0xdeadbeef);reinterpret_cast<Vec>(0x10017cc0)(object.data(),guard.data(),int(i+2));for(unsigned j=i+2;j<guard.size();++j)need(guard[j]==0xdeadbeef,"vector requested write extent guard");std::copy_n(guard.begin(),i+2,vectors[i].begin());}
  using Mat=const uint32_t*(__thiscall*)(void*);const auto*pointer=reinterpret_cast<Mat>(0x10018d60)(object.data());need(pointer!=nullptr,"matrix getter pointer");std::copy_n(pointer,16,matrix.begin());
  need(object==before&&fixture.queues==6,"cached getters unchanged object/null-queue calls");using Clear=void(__thiscall*)(void*);reinterpret_cast<Clear>(0x1001a0a0)(object.data());need(fixture.allocations==fixture.releases,"cached getter backing balanced");totalAlloc+=fixture.allocations;totalFree+=fixture.releases;fixture.image.verify();
#else
  auto retained=source(in);const auto type=retained.value.value.numeric.typeAndMetadata&15u;need(materialStringDomainCandidate(type)||type==7,"owned cached domain");
  const auto&numeric=retained.value.value.numeric;cache[0]=numeric.typeAndMetadata&0xffffu;cache[1]=numeric.integer;std::copy(numeric.vector.begin(),numeric.vector.end(),cache.begin()+2);
  MaterialParameterCollectionCandidate parameters;parameters.values.push_back(retained.value);MaterialParameterBackupServicesCandidate service(parameters);integer=service.integer(1);scalar=service.scalar(1);for(unsigned i=0;i<3;++i)vectors[i]=service.vector(1,i+2);matrix=service.matrix(1);
#endif
  for(auto w:cache)put(out,w);put(out,integer);put(out,scalar);for(const auto&v:vectors)for(auto w:v)put(out,w);for(auto w:matrix)put(out,w);
 }
 need(in.peek()==std::char_traits<char>::eof(),"cached query tail");out.flush();need(bool(out),"complete getter output");_mm_setcsr(oldMx);
#ifdef SFM64_NATIVE_CACHED_BACKUP
 _controlfp_s(&control,previous,_MCW_RC|_MCW_PC);std::cout<<"allocations="<<totalAlloc<<" releases="<<totalFree<<'\n';
#endif
 std::cout<<"PASS cached_getter_cases="<<cases<<'\n';return 0;
}
#ifndef SFM64_NATIVE_CACHED_BACKUP
static int capture(const char*image,const char*sourcesPath,const char*query,const char*destination){
 need(!std::ifstream(image).good()&&!std::ifstream(destination).good(),"absent image fresh output");std::ifstream src(sourcesPath,std::ios::binary),in(query,std::ios::binary);std::ofstream out(destination,std::ios::binary);need(bool(src)&&bool(in)&&bool(out),"capture IO");
 auto n=read(src);need(n&&n<20000,"source count");std::vector<Source>sources;for(unsigned i=0;i<n;++i)sources.push_back(source(src));need(src.peek()==std::char_traits<char>::eof(),"source tail");auto count=read(in);need(count&&count<100000,"case count");put(out,count);unsigned total=0;
 for(unsigned ci=0;ci<count;++ci){auto flags=read(in);const auto phaseBits=read(in);int32_t phase;std::memcpy(&phase,&phaseBits,4);auto rowsCount=read(in);need(rowsCount<=256,"capture template count");MaterialParameterCollectionCandidate parameters;std::vector<MaterialParameterResourceTextCandidate>resources;std::vector<MaterialSessionOverrideInputCandidate>rows;
  for(unsigned i=0;i<rowsCount;++i){MaterialSessionOverrideInputCandidate row;row.typeFlags=read(in);const auto presence=read(in),nameState=read(in);for(auto&w:row.words)w=read(in);std::array<uint32_t,16>poison{};for(auto&w:poison)w=read(in);row.name=text(in);row.textureName=text(in);auto ignored=text(in);(void)ignored;need(presence==3&&nameState==0&&poison[15]<sources.size(),"owned mapping");auto retained=sources[poison[15]];retained.value.name=row.name;parameters.values.push_back(std::move(retained.value));resources.push_back(std::move(retained.resource));rows.push_back(std::move(row));}
  Trace service(parameters,resources);if((flags&3)==3)need(captureMaterialSessionBackupCandidate(rows,service,phase),"cached typed capture");put(out,ci);put(out,rowsCount);for(const auto&row:rows){put(out,row.typeFlags);for(auto w:row.words)put(out,w);text(out,(row.typeFlags&31u)==5?*row.textureName:std::string());}put(out,uint32_t(service.events.size()));for(const auto&e:service.events){put(out,e.kind);put(out,e.index);put(out,e.count);}total+=uint32_t(service.events.size());
 }
 need(in.peek()==std::char_traits<char>::eof(),"capture query tail");out.flush();need(bool(out),"capture output complete");std::cout<<"PASS cached_capture_cases="<<count<<" events="<<total<<'\n';return 0;
}
#endif
int main(int argc,char**argv){try{
 if(argc==5&&std::string(argv[1])=="--getters")return getters(argv[2],argv[3],argv[4]);
#ifndef SFM64_NATIVE_CACHED_BACKUP
 if(argc==6&&std::string(argv[1])=="--capture")return capture(argv[2],argv[3],argv[4],argv[5]);
#endif
 throw std::runtime_error("--getters image source-query fresh-output OR --capture absent-image source-query owned-query fresh-output");
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
