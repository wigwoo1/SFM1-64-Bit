// Complete original collection traversal + capture + apply with explicit host
// materials/getters/setters/resources. No engine startup, code patches or GPU.
#define NOMINMAX
#include <windows.h>
#include <array>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#ifdef SFM64_NATIVE_SESSION_COLLECTION
#include "native_private_world_image_fixture.h"
#else
#include "material_session_collection_candidate.h"
#include "material_session_backup_candidate.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&in){uint32_t v=0;need(bool(in.read(reinterpret_cast<char*>(&v),4)),"Query word");return v;}
static void put(std::ostream&out,uint32_t v){out.write(reinterpret_cast<const char*>(&v),4);}
static std::string readText(std::istream&in){const auto n=read(in);need(n<=4095,"Text extent");std::string s(n,'\0');need(bool(in.read(s.data(),n)),"Query text");return s;}
static void putText(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
struct State{uint32_t presence=3;std::array<uint32_t,16>words{};std::string text;};
struct Material{uint32_t refs=0;std::array<State,2>vars;};
struct Attribute{uint32_t flags=0,nameState=0;std::array<uint32_t,16>words{};std::string name,text;};
struct Element{uint32_t available=1,type=1,cached=0,lookup=0,nameState=0;std::string name;std::vector<Attribute>attrs;};
struct Event{uint32_t kind=0,a=0,b=0,c=0;std::array<uint32_t,16>words{};};
static std::vector<Material>materials;static std::vector<Element>elements;static std::vector<Event>events;
static uint32_t emptyReturn=0;static int phase=0;
static void event(uint32_t kind,uint32_t a,uint32_t b=0,uint32_t c=0,const uint32_t*words=nullptr){Event e;e.kind=kind;e.a=a;e.b=b;e.c=c;if(words)std::copy_n(words,c,e.words.begin());events.push_back(e);}
static uint32_t nameIndex(const char*s){return !std::strcmp(s,"$a")?0:!std::strcmp(s,"$b")?1:UINT32_MAX;}
static uint32_t lookupMaterial(const char*name){need(name!=nullptr,"FindMaterial nonnull name");uint32_t id=0,result=emptyReturn;if(*name){for(size_t i=0;i<elements.size();++i)if(elements[i].name==name){id=uint32_t(i+1);result=elements[i].lookup;break;}need(id!=0,"Explicit material lookup");}event(2,id,result);return result;}
static uint32_t lookupTexture(const char*name){need(name!=nullptr,"Controlled nonnull texture name");const auto text=std::string(name);uint32_t token=text.empty()?0:text=="texA"?101:text=="texB"?102:103;event(13,token);return token;}
static std::string textureText(uint32_t token){return token==0?"":token==101?"texA":token==102?"texB":"other";}
static State&state(uint32_t m,uint32_t v){need(m&&m<=materials.size()&&v<2,"Variable identity");return materials[m-1].vars[v];}
static void setWords(uint32_t kind,uint32_t m,uint32_t v,const uint32_t*p,uint32_t n){event(kind,m,v,n,p);std::copy_n(p,n,state(m,v).words.begin());}
static void setResource(uint32_t m,uint32_t v,uint32_t token){event(14,m,v,1,&token);auto&s=state(m,v);s.words[0]=token;s.text=textureText(token);}
#ifdef SFM64_NATIVE_SESSION_COLLECTION
struct NativeMaterial{uintptr_t methods;uint32_t id;};struct NativeVariable{uintptr_t methods;uint32_t material,index;};
static std::vector<NativeMaterial>nativeMaterials;static std::vector<std::array<NativeVariable,2>>nativeVars;
static std::vector<std::array<uint8_t,0x64>>nativeElements;
static uint32_t elementId(void*p){for(size_t i=0;i<nativeElements.size();++i)if(nativeElements[i].data()==p)return uint32_t(i+1);throw std::runtime_error("Element identity");}
static void*__fastcall resolve(void*,void*,uint32_t handle){const auto id=handle&&handle<=elements.size()&&elements[handle-1].available?handle:0;event(0,handle,id);return id?nativeElements[id-1].data():nullptr;}
static bool __fastcall materialType(void*p,void*,uint32_t symbol){need(symbol==0x12345678,"DmeMaterial type symbol");const auto id=elementId(p);event(1,id,elements[id-1].type);return elements[id-1].type!=0;}
static void*__fastcall findMaterial(void*,void*,const char*name,const char*group,bool complain,const char*prefix){need(!group&&!complain&&!prefix,"Actual FindMaterial arguments");const auto id=lookupMaterial(name);return id?&nativeMaterials[id-1]:nullptr;}
static void __fastcall addRef(void*p,void*){const auto id=static_cast<NativeMaterial*>(p)->id;++materials[id-1].refs;event(15,id);}
static void __fastcall releaseRef(void*,void*){throw std::runtime_error("Unexpected release in lazy acquisition");}
static void*__fastcall findVar(void*p,void*,const char*name,bool*found,bool complain){need(!complain,"FindVar complaint flag");const auto m=static_cast<NativeMaterial*>(p)->id,v=nameIndex(name);const auto presence=v<2?state(m,v).presence:0;*found=bool(presence&2);event(3,m,v,presence);return presence&1?&nativeVars[m-1][v]:nullptr;}
static NativeVariable&var(void*p){return *static_cast<NativeVariable*>(p);}
static uint32_t __fastcall getInt(void*p,void*){auto&v=var(p);event(4,v.material,v.index);return state(v.material,v.index).words[0];}
static float __fastcall getFloat(void*p,void*){auto&v=var(p);event(5,v.material,v.index);float f;auto word=state(v.material,v.index).words[0];std::memcpy(&f,&word,4);return f;}
static void __fastcall getVector(void*p,void*,uint32_t*out,int n){auto&v=var(p);need(n>=2&&n<=4,"Vector getter extent");event(6,v.material,v.index,uint32_t(n));std::copy_n(state(v.material,v.index).words.begin(),n,out);}
static const uint32_t*__fastcall getMatrix(void*p,void*){auto&v=var(p);event(7,v.material,v.index,16);return state(v.material,v.index).words.data();}
static const char*__fastcall getString(void*p,void*){auto&v=var(p);event(8,v.material,v.index);return state(v.material,v.index).text.c_str();}
static void __fastcall setInt(void*p,void*,uint32_t w){auto&v=var(p);setWords(9,v.material,v.index,&w,1);}
static void __fastcall setFloat(void*p,void*,uint32_t w){auto&v=var(p);setWords(10,v.material,v.index,&w,1);}
static void __fastcall setVector2(void*p,void*,uint32_t a,uint32_t b){auto&v=var(p);uint32_t w[]={a,b};setWords(11,v.material,v.index,w,2);}
static void __fastcall setVector3(void*p,void*,uint32_t a,uint32_t b,uint32_t c){auto&v=var(p);uint32_t w[]={a,b,c};setWords(11,v.material,v.index,w,3);}
static void __fastcall setVector4(void*p,void*,uint32_t a,uint32_t b,uint32_t c,uint32_t d){auto&v=var(p);uint32_t w[]={a,b,c,d};setWords(11,v.material,v.index,w,4);}
static void __fastcall setMatrix(void*p,void*,const uint32_t*w){auto&v=var(p);setWords(12,v.material,v.index,w,16);}
static void*__fastcall findTexture(void*,void*,const char*name,const char*group,bool complain,int flags){need(group&&!*group&&!complain&&!flags,"FindTexture arguments");return reinterpret_cast<void*>(uintptr_t(lookupTexture(name)));}
static void __fastcall setTexture(void*p,void*,void*token){auto&v=var(p);setResource(v.material,v.index,uint32_t(reinterpret_cast<uintptr_t>(token)));}
static std::map<std::string,std::string>strings;
static const char*intern(const char*s){need(s!=nullptr,"Explicit nonnull string intern");return strings.emplace(s,s).first->second.c_str();}
static uint32_t*__fastcall internString(void*,void*,uint32_t*out,const char*s){*out=uint32_t(reinterpret_cast<uintptr_t>(intern(s)));return out;}
static int __fastcall getPhase(void*,void*){return phase;}
static bool __fastcall noUndo(void*,void*,void*){return false;}
static bool __fastcall suppressNotify(void*,void*){return true;}
template<size_t N>static void field(std::array<uint8_t,N>&a,size_t at,uintptr_t value){need(at+4<=N,"Native field extent");const auto word=uint32_t(value);std::memcpy(a.data()+at,&word,4);}
#else
static uintptr_t variableToken(uint32_t m,uint32_t v){return uintptr_t(m*2+v+1);}
static std::pair<uint32_t,uint32_t>variableId(uintptr_t p){need(p>=3,"Variable token");return {uint32_t((p-1)/2),uint32_t((p-1)%2)};}
static MaterialSessionOverrideLookupCandidate findValue(uint32_t m,std::string_view name){const auto v=nameIndex(std::string(name).c_str());const auto presence=v<2?state(m,v).presence:0;event(3,m,v,presence);return {presence&1?variableToken(m,v):0,bool(presence&2)};}
struct CaptureServices:MaterialSessionBackupServicesCandidate{
 uint32_t material=0;
 MaterialSessionOverrideLookupCandidate find(std::string_view name)override{return findValue(material,name);}
 uint32_t integer(uintptr_t p)override{auto[m,v]=variableId(p);event(4,m,v);return state(m,v).words[0];}
 uint32_t scalar(uintptr_t p)override{auto[m,v]=variableId(p);event(5,m,v);return state(m,v).words[0];}
 std::array<uint32_t,4>vector(uintptr_t p,uint32_t n)override{auto[m,v]=variableId(p);event(6,m,v,n);std::array<uint32_t,4>w{};std::copy_n(state(m,v).words.begin(),n,w.begin());return w;}
 std::array<uint32_t,16>matrix(uintptr_t p)override{auto[m,v]=variableId(p);event(7,m,v,16);return state(m,v).words;}
 std::optional<std::string>stringValue(uintptr_t p)override{auto[m,v]=variableId(p);event(8,m,v);return state(m,v).text;}
};
struct ApplyServices:MaterialSessionOverrideServicesCandidate{
 uint32_t material=0;
 MaterialSessionOverrideLookupCandidate find(std::string_view name)override{return findValue(material,name);}
 void integer(uintptr_t p,uint32_t w)override{auto[m,v]=variableId(p);setWords(9,m,v,&w,1);}
 void scalar(uintptr_t p,uint32_t w)override{auto[m,v]=variableId(p);setWords(10,m,v,&w,1);}
 void vector(uintptr_t p,const std::array<uint32_t,4>&w,uint32_t n)override{auto[m,v]=variableId(p);setWords(11,m,v,w.data(),n);}
 void matrix(uintptr_t p,const std::array<uint32_t,16>&w)override{auto[m,v]=variableId(p);setWords(12,m,v,w.data(),16);}
 std::optional<uintptr_t>texture(const std::optional<std::string>&name)override{if(!name)return {};return lookupTexture(name->c_str());}
 void setTexture(uintptr_t p,uintptr_t token)override{auto[m,v]=variableId(p);setResource(m,v,uint32_t(token));}
};
static std::vector<MaterialSessionOverrideInputCandidate>typedRows(const Element&e){std::vector<MaterialSessionOverrideInputCandidate>r;for(auto&a:e.attrs){MaterialSessionOverrideInputCandidate v;v.name=a.name;v.nameAvailable=a.nameState==0;v.typeFlags=a.flags;v.words=a.words;v.textureName=a.text;r.push_back(std::move(v));}return r;}
struct CollectionServices:MaterialSessionCollectionServicesCandidate,MaterialSessionResourceServicesCandidate{
 std::vector<MaterialSessionCachedResourceCandidate>bindings;
 uintptr_t resolve(uint32_t h)override{const auto id=h&&h<=elements.size()&&elements[h-1].available?h:0;event(0,h,id);return id;}
 bool isMaterial(uintptr_t p)override{event(1,uint32_t(p),elements[p-1].type);return elements[p-1].type!=0;}
 std::optional<uintptr_t>acquire(uintptr_t p)override{auto r=acquireMaterialSessionResourceCandidate(bindings[p-1],*this);if(r)elements[p-1].cached=uint32_t(*r);return r;}
 std::optional<uintptr_t>findMaterial(std::string_view s)override{return lookupMaterial(std::string(s).c_str());}
 void addReference(uintptr_t p)override{++materials[p-1].refs;event(15,uint32_t(p));}
 bool capture(uintptr_t backup,uintptr_t m)override{if(!backup)return true;auto&e=elements[backup-1];auto rows=typedRows(e);CaptureServices s;s.material=uint32_t(m);if(!captureMaterialSessionBackupCandidate(rows,s,phase))return false;for(size_t i=0;i<rows.size();++i){e.attrs[i].flags=rows[i].typeFlags;e.attrs[i].words=rows[i].words;e.attrs[i].text=*rows[i].textureName;}return true;}
 bool apply(uintptr_t source,uintptr_t m)override{ApplyServices s;s.material=uint32_t(m);return dispatchMaterialSessionOverridesCandidate(typedRows(elements[source-1]),s);}
};
#endif
static void snapshot(std::ostream&out){
 for(const auto&m:materials){put(out,m.refs);for(const auto&v:m.vars){for(auto w:v.words)put(out,w);putText(out,v.text);}}
 for(const auto&e:elements){put(out,e.cached);for(const auto&a:e.attrs){put(out,a.flags);for(auto w:a.words)put(out,w);putText(out,(a.flags&31)==5?std::string(a.text.c_str()):std::string());}}
}
int main(int argc,char**argv){try{
 need(argc==4,"private-or-absent-image query fresh-output");need(!std::ifstream(argv[3]).good(),"Fresh output");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"Probe IO");
#ifdef SFM64_NATIVE_SESSION_COLLECTION
 static_assert(sizeof(void*)==4,"Private reference x86");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);NativePrivateWorldImageFixture image(argv[1]);
 std::array<uintptr_t,120>dmMethods{};dmMethods[18]=reinterpret_cast<uintptr_t>(&resolve);dmMethods[50]=reinterpret_cast<uintptr_t>(&internString);dmMethods[62]=reinterpret_cast<uintptr_t>(&noUndo);dmMethods[119]=reinterpret_cast<uintptr_t>(&suppressNotify);std::array<uint8_t,0xc184>dm{};field(dm,0,reinterpret_cast<uintptr_t>(dmMethods.data()));const auto dmPtr=reinterpret_cast<uintptr_t>(dm.data());std::memcpy(image.base+0xe9a51c,&dmPtr,4);std::memcpy(image.base+0xe9a518,&dmPtr,4);
 std::array<uintptr_t,9>phaseMethods{};phaseMethods[8]=reinterpret_cast<uintptr_t>(&getPhase);uintptr_t phaseObject=reinterpret_cast<uintptr_t>(phaseMethods.data());const auto phasePtr=reinterpret_cast<uintptr_t>(&phaseObject);std::memcpy(image.base+0xe9ccf8,&phasePtr,4);
 std::array<uintptr_t,84>systemMethods{};systemMethods[77]=reinterpret_cast<uintptr_t>(&findMaterial);systemMethods[83]=reinterpret_cast<uintptr_t>(&findTexture);uintptr_t system=reinterpret_cast<uintptr_t>(systemMethods.data());const auto systemPtr=reinterpret_cast<uintptr_t>(&system);std::memcpy(image.base+0xf4c778,&systemPtr,4);
 const uint32_t symbol=0x12345678;std::memcpy(image.base+0xeb4ec8,&symbol,4);
 std::array<uintptr_t,29>varMethods{};varMethods[3]=reinterpret_cast<uintptr_t>(&setFloat);varMethods[4]=reinterpret_cast<uintptr_t>(&setInt);varMethods[6]=reinterpret_cast<uintptr_t>(&getString);varMethods[9]=reinterpret_cast<uintptr_t>(&setVector4);varMethods[10]=reinterpret_cast<uintptr_t>(&setVector3);varMethods[11]=reinterpret_cast<uintptr_t>(&setVector2);varMethods[14]=reinterpret_cast<uintptr_t>(&setTexture);varMethods[19]=reinterpret_cast<uintptr_t>(&setMatrix);varMethods[20]=reinterpret_cast<uintptr_t>(&getMatrix);varMethods[26]=reinterpret_cast<uintptr_t>(&getInt);varMethods[27]=reinterpret_cast<uintptr_t>(&getFloat);varMethods[28]=reinterpret_cast<uintptr_t>(&getVector);
 std::array<uintptr_t,14>matMethods{};matMethods[11]=reinterpret_cast<uintptr_t>(&findVar);matMethods[12]=reinterpret_cast<uintptr_t>(&addRef);matMethods[13]=reinterpret_cast<uintptr_t>(&releaseRef);std::array<uintptr_t,5>elementMethods{};elementMethods[4]=reinterpret_cast<uintptr_t>(&materialType);image.seal();
#else
 static_assert(sizeof(void*)==8,"Owned collection x64");need(!std::ifstream(argv[1]).good(),"Owned cannot consume engine image");
#endif
 const auto cases=read(in);need(cases&&cases<100000,"Cases extent");put(out,cases);uint32_t total=0;
 for(uint32_t ci=0;ci<cases;++ci){const auto nm=read(in),ne=read(in),ns=read(in),nb=read(in),flags=read(in);emptyReturn=read(in);phase=int(read(in));need(nm&&nm<=16&&ne<=64&&ns<=256&&nb<=256&&emptyReturn<=nm,"Case extents");materials.assign(nm,{});elements.assign(ne,{});events.clear();
  for(auto&m:materials)for(auto&v:m.vars){v.presence=read(in);need(v.presence<4,"Variable presence");for(auto&w:v.words)w=read(in);v.text=readText(in);}
  for(size_t i=0;i<ne;++i){auto&e=elements[i];e.available=read(in);e.type=read(in);e.cached=read(in);e.lookup=read(in);e.nameState=read(in);const auto na=read(in);need(e.available<2&&e.type<2&&e.cached<=nm&&e.lookup<=nm&&e.nameState<3&&na<=64,"Element shape");e.name="e"+std::to_string(i+1);e.attrs.resize(na);for(auto&a:e.attrs){a.flags=read(in);a.nameState=read(in);need(a.nameState<3&&(!(a.flags&0x2000)||(a.flags&0x80)),"Attribute shape");for(auto&w:a.words)w=read(in);a.name=readText(in);a.text=readText(in);}}
  std::vector<uint32_t>source(ns),backup(nb);for(auto&h:source)h=read(in);for(auto&h:backup)h=read(in);
#ifdef SFM64_NATIVE_SESSION_COLLECTION
  strings.clear();nativeMaterials.resize(nm);nativeVars.resize(nm);nativeElements.assign(ne,{});std::vector<std::vector<std::array<uint8_t,24>>>attrs(ne);std::vector<std::vector<std::array<uint32_t,16>>>values(ne);
  for(size_t i=0;i<nm;++i){nativeMaterials[i]={reinterpret_cast<uintptr_t>(matMethods.data()),uint32_t(i+1)};for(unsigned j=0;j<2;++j)nativeVars[i][j]={reinterpret_cast<uintptr_t>(varMethods.data()),uint32_t(i+1),j};}
  for(size_t i=0;i<ne;++i){const auto&e=elements[i];auto&block=nativeElements[i];field(block,0,reinterpret_cast<uintptr_t>(elementMethods.data()));block[0x3e]=1;field(block,0x5c,e.cached?reinterpret_cast<uintptr_t>(&nativeMaterials[e.cached-1]):0);field(block,0x60,e.nameState==1?0:e.nameState==2?UINT32_MAX:reinterpret_cast<uintptr_t>(e.name.c_str()));attrs[i].resize(e.attrs.size());values[i].resize(e.attrs.size());for(size_t j=0;j<e.attrs.size();++j){const auto&a=e.attrs[j];values[i][j]=a.words;field(attrs[i][j],0,j+1<e.attrs.size()?reinterpret_cast<uintptr_t>(attrs[i][j+1].data()):0);field(attrs[i][j],4,reinterpret_cast<uintptr_t>(values[i][j].data()));field(attrs[i][j],8,reinterpret_cast<uintptr_t>(block.data()));field(attrs[i][j],0x10,a.flags);field(attrs[i][j],0x14,a.nameState==1?0:a.nameState==2?UINT32_MAX:reinterpret_cast<uintptr_t>(a.name.c_str()));if((a.flags&31)==5)values[i][j][0]=uint32_t(reinterpret_cast<uintptr_t>(intern(a.text.c_str())));}field(block,0x30,e.attrs.empty()?0:reinterpret_cast<uintptr_t>(attrs[i][0].data()));}
  std::array<uint8_t,16>sourceArray{},backupArray{};field(sourceArray,0,reinterpret_cast<uintptr_t>(source.data()));field(sourceArray,12,source.size());field(backupArray,0,reinterpret_cast<uintptr_t>(backup.data()));field(backupArray,12,backup.size());
  auto sync=[&](){for(size_t i=0;i<ne;++i){uint32_t cache=0;std::memcpy(&cache,nativeElements[i].data()+0x5c,4);elements[i].cached=cache?static_cast<NativeMaterial*>(reinterpret_cast<void*>(uintptr_t(cache)))->id:0;for(size_t j=0;j<elements[i].attrs.size();++j){auto&a=elements[i].attrs[j];std::memcpy(&a.flags,attrs[i][j].data()+0x10,4);a.words=values[i][j];if((a.flags&31)==5){a.text=reinterpret_cast<const char*>(uintptr_t(values[i][j][0]));a.words[0]=0;}}}};
  using Loop=void(__thiscall*)(void*,void*,void*);auto invoke=[&](bool restore){reinterpret_cast<Loop>(0x1023e3e0)(nullptr,restore?backupArray.data():sourceArray.data(),restore?nullptr:flags&1?backupArray.data():nullptr);sync();image.verify();};
#else
  CollectionServices service;for(auto&e:elements){MaterialSessionCachedResourceCandidate b;b.resource=e.cached;if(e.nameState!=1)b.name=e.nameState==2?std::string():e.name;service.bindings.push_back(std::move(b));}auto invoke=[&](bool restore){need(applyMaterialSessionCollectionCandidate(restore?backup:source,restore?nullptr:flags&1?&backup:nullptr,service),"Owned collection scope");};
#endif
  // String attrs store an engine symbol pointer in native data. Their numeric
  // word0 is not observable semantic value; normalize only that word on output.
  for(auto&e:elements)for(auto&a:e.attrs)if((a.flags&31)==5)a.words[0]=0;
  put(out,ci);const auto repeats=flags&4?2u:1u;put(out,repeats*((flags&3)==3?2u:1u));
  for(unsigned repeat=0;repeat<repeats;++repeat){invoke(false);snapshot(out);if((flags&3)==3){invoke(true);snapshot(out);}}
  put(out,uint32_t(events.size()));for(const auto&e:events)out.write(reinterpret_cast<const char*>(&e),sizeof(e));total+=uint32_t(events.size());
 }
 need(in.peek()==std::char_traits<char>::eof(),"Query tail");out.flush();need(bool(out),"Output complete");
#ifdef SFM64_NATIVE_SESSION_COLLECTION
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"Private image release");
#endif
 std::cout<<"PASS collection_cases="<<cases<<" ordered_events="<<total<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
