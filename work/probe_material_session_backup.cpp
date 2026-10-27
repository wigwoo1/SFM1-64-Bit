// Test-only value capture. Actual IFM capture and typed CDmAttribute setters
// execute unchanged RX. Host getters/interning/phase/undo/notify are explicit.
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
#ifdef SFM64_NATIVE_SESSION_BACKUP
#include "native_private_world_image_fixture.h"
#else
#include "material_session_backup_candidate.h"
#endif
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&in){uint32_t v=0;need(bool(in.read(reinterpret_cast<char*>(&v),4)),"Query word");return v;}
static void put(std::ostream&out,uint32_t v){out.write(reinterpret_cast<const char*>(&v),4);}
static std::string text(std::istream&in){const auto n=read(in);need(n<=4095,"Text bounds");std::string s(n,'\0');need(bool(in.read(s.data(),n)),"Text bytes");return s;}
static void putText(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
struct Row{uint32_t flags=0,presence=3,nameState=0;std::array<uint32_t,16>before{},live{};std::string name,oldText,liveText;};
struct Event{uint32_t kind=0,index=0,count=0;};
static std::vector<Row>rows;static std::vector<Event>events;static int phase=0;
static void event(uint32_t kind,size_t index,uint32_t count=0){events.push_back({kind,uint32_t(index),count});}
#ifdef SFM64_NATIVE_SESSION_BACKUP
struct Variable{uintptr_t methods;uint32_t index;};static std::vector<Variable>vars;
static size_t indexOf(void*p){return static_cast<Variable*>(p)->index;}
static void*__fastcall find(void*,void*,const char*name,bool*found,bool complain){size_t i=0;for(;i<rows.size();++i)if(rows[i].nameState==0&&rows[i].name.c_str()==name)break;need(i<rows.size()&&!complain,"FindVar observer identity");*found=bool(rows[i].presence&2);event(0,i);return rows[i].presence&1?&vars[i]:nullptr;}
static uint32_t __fastcall getInt(void*p,void*){const auto i=indexOf(p);event(1,i);return rows[i].live[0];}
static float __fastcall getFloat(void*p,void*){const auto i=indexOf(p);event(2,i);float v;std::memcpy(&v,&rows[i].live[0],4);return v;}
static void __fastcall getVector(void*p,void*,uint32_t*out,int n){const auto i=indexOf(p);need(n>=2&&n<=4,"Getter vector count");event(3,i,uint32_t(n));std::copy_n(rows[i].live.begin(),n,out);}
static const uint32_t*__fastcall getMatrix(void*p,void*){const auto i=indexOf(p);event(4,i,16);return rows[i].live.data();}
static const char*__fastcall getString(void*p,void*){const auto i=indexOf(p);event(5,i);return rows[i].liveText.c_str();}
static std::map<std::string,std::string>strings;
static const char*intern(const char*s){need(s!=nullptr,"Explicit nonnull string fixture");auto it=strings.emplace(s,s).first;return it->second.c_str();}
static uint32_t*__fastcall internService(void*,void*,uint32_t*out,const char*s){*out=uint32_t(reinterpret_cast<uintptr_t>(intern(s)));return out;}
static uint32_t phaseCalls=0,undoCalls=0,notifyCalls=0;
static int __fastcall getPhase(void*,void*){++phaseCalls;return phase;}
static bool __fastcall undoEnabled(void*,void*,void*){++undoCalls;return false;}
static bool __fastcall suppressNotify(void*,void*){++notifyCalls;return true;}
template<size_t N>static void field(std::array<uint8_t,N>&a,size_t at,uintptr_t value){need(at+4<=N,"Native field bounds");const auto v=uint32_t(value);std::memcpy(a.data()+at,&v,4);}
#else
struct Services:MaterialSessionBackupServicesCandidate{
 size_t cursor=0;
 MaterialSessionOverrideLookupCandidate find(std::string_view name)override{size_t i=cursor;for(;i<rows.size();++i)if(rows[i].nameState==0&&std::string_view(rows[i].name.c_str())==name)break;need(i<rows.size(),"Owned ordered lookup identity");cursor=i+1;event(0,i);return {rows[i].presence&1?i+1:0,bool(rows[i].presence&2)};}
 uint32_t integer(uintptr_t p)override{event(1,p-1);return rows[p-1].live[0];}
 uint32_t scalar(uintptr_t p)override{event(2,p-1);return rows[p-1].live[0];}
 std::array<uint32_t,4>vector(uintptr_t p,uint32_t n)override{event(3,p-1,n);std::array<uint32_t,4>v{};std::copy_n(rows[p-1].live.begin(),n,v.begin());return v;}
 std::array<uint32_t,16>matrix(uintptr_t p)override{event(4,p-1,16);return rows[p-1].live;}
 std::optional<std::string>stringValue(uintptr_t p)override{event(5,p-1);return rows[p-1].liveText;}
};
#endif
int main(int argc,char**argv){try{
 need(argc==4,"private-or-absent-image query fresh-output");need(!std::ifstream(argv[3]).good(),"Fresh output required");std::ifstream in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);need(bool(in)&&bool(out),"File IO");
#ifdef SFM64_NATIVE_SESSION_BACKUP
 static_assert(sizeof(void*)==4,"Private reference x86");SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);NativePrivateWorldImageFixture image(argv[1]);
 std::array<uintptr_t,120>dmMethods{};dmMethods[50]=reinterpret_cast<uintptr_t>(&internService);dmMethods[62]=reinterpret_cast<uintptr_t>(&undoEnabled);dmMethods[119]=reinterpret_cast<uintptr_t>(&suppressNotify);
 std::array<uint8_t,0xc184>dm{};field(dm,0,reinterpret_cast<uintptr_t>(dmMethods.data()));const auto dmPtr=reinterpret_cast<uintptr_t>(dm.data());std::memcpy(image.base+0xe9a51c,&dmPtr,4);std::memcpy(image.base+0xe9a518,&dmPtr,4);
 std::array<uintptr_t,9>phaseMethods{};phaseMethods[8]=reinterpret_cast<uintptr_t>(&getPhase);uintptr_t phaseObject=reinterpret_cast<uintptr_t>(phaseMethods.data());const auto phasePtr=reinterpret_cast<uintptr_t>(&phaseObject);std::memcpy(image.base+0xe9ccf8,&phasePtr,4);
 std::array<uintptr_t,29>varMethods{};varMethods[6]=reinterpret_cast<uintptr_t>(&getString);varMethods[20]=reinterpret_cast<uintptr_t>(&getMatrix);varMethods[26]=reinterpret_cast<uintptr_t>(&getInt);varMethods[27]=reinterpret_cast<uintptr_t>(&getFloat);varMethods[28]=reinterpret_cast<uintptr_t>(&getVector);
 std::array<uintptr_t,12>materialMethods{};materialMethods[11]=reinterpret_cast<uintptr_t>(&find);uintptr_t material=reinterpret_cast<uintptr_t>(materialMethods.data());image.seal();
#else
 static_assert(sizeof(void*)==8,"Owned x64 value capture");need(!std::ifstream(argv[1]).good(),"Owned must not read an engine image");
#endif
 const auto cases=read(in);need(cases&&cases<100000,"Case count");put(out,cases);uint32_t total=0;
 for(uint32_t c=0;c<cases;++c){const auto flags=read(in);phase=int(read(in));const auto n=read(in);need(n<=4096,"Row bounds");rows.assign(n,{});events.clear();
  for(auto&r:rows){r.flags=read(in);r.presence=read(in);r.nameState=read(in);for(auto&w:r.before)w=read(in);for(auto&w:r.live)w=read(in);r.name=text(in);r.oldText=text(in);r.liveText=text(in);need(r.presence<4&&r.nameState<3,"Query states");need(!(r.flags&0x2000)||(r.flags&0x80),"Unrecovered owner-prepare fixture forbidden");}
  std::vector<std::array<uint32_t,16>>values(n);std::vector<uint32_t>types(n);std::vector<std::string>texts(n);
#ifdef SFM64_NATIVE_SESSION_BACKUP
  std::vector<std::array<uint8_t,24>>attrs(n);std::array<uint8_t,0x48>owner{};owner[0x3e]=1;std::array<uint8_t,0x34>element{};vars.resize(n);strings.clear();
  for(size_t i=0;i<n;++i){values[i]=rows[i].before;vars[i]={reinterpret_cast<uintptr_t>(varMethods.data()),uint32_t(i)};field(attrs[i],0,i+1<n?reinterpret_cast<uintptr_t>(attrs[i+1].data()):0);field(attrs[i],4,reinterpret_cast<uintptr_t>(values[i].data()));field(attrs[i],8,reinterpret_cast<uintptr_t>(owner.data()));field(attrs[i],0x10,rows[i].flags);field(attrs[i],0x14,rows[i].nameState==1?0:rows[i].nameState==2?UINT32_MAX:reinterpret_cast<uintptr_t>(rows[i].name.c_str()));if((rows[i].flags&31)==5)values[i][0]=uint32_t(reinterpret_cast<uintptr_t>(intern(rows[i].oldText.c_str())));}
  field(element,0x30,n?reinterpret_cast<uintptr_t>(attrs[0].data()):0);std::fill(dm.begin()+0xc180,dm.end(),uint8_t(0));using Capture=void(__cdecl*)(void*,void*);reinterpret_cast<Capture>(0x1023c0a0)(flags&1?element.data():nullptr,flags&2?&material:nullptr);image.verify();
  for(size_t i=0;i<n;++i){std::memcpy(&types[i],attrs[i].data()+0x10,4);if((types[i]&31)==5){texts[i]=reinterpret_cast<const char*>(uintptr_t(values[i][0]));values[i][0]=rows[i].before[0];}}
  uint32_t mutation=0;std::memcpy(&mutation,dm.data()+0xc180,4);need((owner[0x3e]==1&&mutation==0)||(owner[0x3e]==3&&mutation==0x2000),"Native owner/model write markers");
#else
  std::vector<MaterialSessionOverrideInputCandidate>input;input.reserve(n);for(auto&r:rows){MaterialSessionOverrideInputCandidate v;v.name=r.name;v.nameAvailable=r.nameState==0;v.typeFlags=r.flags;v.words=r.before;v.textureName=r.oldText;input.push_back(std::move(v));}Services service;if((flags&3)==3)need(captureMaterialSessionBackupCandidate(input,service,phase),"Owned capture");for(size_t i=0;i<n;++i){values[i]=input[i].words;types[i]=input[i].typeFlags;const auto&s=*input[i].textureName;texts[i]=s.substr(0,s.find('\0'));}
#endif
  put(out,c);put(out,n);for(size_t i=0;i<n;++i){put(out,types[i]);for(auto w:values[i])put(out,w);putText(out,(types[i]&31)==5?texts[i]:std::string());}put(out,uint32_t(events.size()));for(const auto&e:events){put(out,e.kind);put(out,e.index);put(out,e.count);}total+=uint32_t(events.size());
 }
 need(in.peek()==std::char_traits<char>::eof(),"Query tail");out.flush();need(bool(out),"Output complete");
#ifdef SFM64_NATIVE_SESSION_BACKUP
 image.verify();need(VirtualFree(image.base,0,MEM_RELEASE)!=0,"Private image release");std::cout<<"phase_calls="<<phaseCalls<<" undo_disabled_calls="<<undoCalls<<" notifications_suppressed="<<notifyCalls<<'\n';
#endif
 std::cout<<"PASS backup_cases="<<cases<<" getter_events="<<total<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
