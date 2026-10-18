#include "source_bsp_collision_surface_table_candidate.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <iostream>
#ifdef NATIVE_BSP_SURFACE_TABLE
#define NOMINMAX
#include <windows.h>
#include "native_private_world_image_fixture.h"
#include "native_vphysics_library_fixture.h"
#endif
template<class T>static T read(std::istream&in){T v;if(!in.read(reinterpret_cast<char*>(&v),sizeof(v)))throw std::runtime_error("surface fixture truncated");return v;}
static std::string string(std::istream&in){auto n=read<uint32_t>(in);if(n>1u<<24)throw std::runtime_error("fixture string extent");std::string s(n,'\0');if(!in.read(s.data(),n))throw std::runtime_error("fixture string truncated");return s;}
template<class T>static void put(std::ostream&out,T value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));}
static void putString(std::ostream&out,const std::string&s){put(out,uint32_t(s.size()));out.write(s.data(),s.size());}
#ifdef NATIVE_BSP_SURFACE_TABLE
struct NativeVar{uintptr_t*table;const std::string*value;};
struct NativeMaterial{uintptr_t*table;const SourceBspCollisionMaterialParametersCandidate*data;std::array<NativeVar,2>vars;};
static std::vector<NativeMaterial>materials;static std::array<uintptr_t,78>systemTable{};static std::array<uintptr_t,43>materialTable{};static std::array<uintptr_t,7>varTable{};static uint32_t requests=0,findVars=0,stringGets=0;
static void* __fastcall findMaterial(void*,void*,const char*name,const char*group,bool complain,const char*prefix){if(std::strcmp(group,"World textures")||!complain||prefix)ExitProcess(91);++requests;for(auto&m:materials)if(m.data->name==name)return m.data->available?&m:nullptr;ExitProcess(92);}
static bool __fastcall isError(NativeMaterial*m,void*){return m->data->error;}
static NativeVar* __fastcall findVar(NativeMaterial*m,void*,const char*name,bool*found,bool complain){if(complain)ExitProcess(93);++findVars;size_t index;if(!std::strcmp(name,"$surfaceprop"))index=0;else if(!std::strcmp(name,"$surfaceprop2"))index=1;else ExitProcess(94);const auto&value=index?m->data->secondary:m->data->primary;*found=value.has_value();return &m->vars[index];}
static const char* __fastcall getString(NativeVar*v,void*){if(!v->value)ExitProcess(95);++stringGets;return v->value->c_str();}
__declspec(naked) static void invokeTextures(void*,void*,void*,void*,uint32_t){__asm{
 push ebp
 mov ebp,esp
 push ebx
 push esi
 push edi
 sub esp,400h
 mov esi,dword ptr [ebp+0ch]
 mov eax,dword ptr [ebp+10h]
 add eax,0ch
 mov dword ptr [ebp-36ch],eax
 mov dword ptr [ebp-370h],0
 mov eax,dword ptr [ebp+14h]
 mov dword ptr [ebp-374h],eax
 mov eax,dword ptr [ebp+18h]
 mov dword ptr [ebp-378h],eax
 call dword ptr [ebp+8]
 lea esp,[ebp-0ch]
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret
}}
__declspec(naked) static void invokeDisplacement(void*,void*,void*,uint32_t){__asm{
 push ebp
 mov ebp,esp
 push ebx
 push esi
 push edi
 sub esp,2900h
 mov eax,dword ptr [ebp+0ch]
 mov dword ptr [ebp-2808h],eax
 mov esi,dword ptr [ebp+10h]
 mov eax,dword ptr [ebp+14h]
 call dword ptr [ebp+8]
 lea esp,[ebp-0ch]
 pop edi
 pop esi
 pop ebx
 pop ebp
 ret
}}
static LONG CALLBACK fault(EXCEPTION_POINTERS*p){if(p->ExceptionRecord->ExceptionCode==0xe06d7363)return EXCEPTION_CONTINUE_SEARCH;std::cerr<<"native surface fault eip="<<std::hex<<p->ContextRecord->Eip<<'\n';ExitProcess(90);}
#endif
int main(int argc,char**argv){try{
 if(argc!=6)throw std::runtime_error("map fixture output engine copied-library-dir required");std::ifstream map(argv[1],std::ios::binary),in(argv[2],std::ios::binary);std::ofstream out(argv[3],std::ios::binary);if(!map||!in||!out)throw std::runtime_error("surface streams");std::vector<uint8_t>b((std::istreambuf_iterator<char>(map)),{});if(b.size()<1036)throw std::runtime_error("map header");auto word=[&](size_t p){if(p>b.size()||b.size()-p<4)throw std::runtime_error("map read span");uint32_t n;std::memcpy(&n,b.data()+p,4);return n;};auto lump=[&](unsigned id,size_t stride){size_t h=8+16*id,o=word(h),n=word(h+4);if(o>b.size()||n>b.size()-o||n%stride)throw std::runtime_error("native fixture lump span");return std::array<size_t,2>{o,n/stride};};
 auto count=read<uint32_t>(in);if(count>4096)throw std::runtime_error("scripts count");std::vector<SourceSurfaceScriptCandidate>scripts;for(uint32_t i=0;i<count;++i){auto name=string(in),text=string(in);scripts.push_back({std::move(name),std::move(text)});}count=read<uint32_t>(in);if(count<1||count>2048)throw std::runtime_error("material count");std::vector<SourceBspCollisionMaterialParametersCandidate>params;for(uint32_t i=0;i<count;++i){SourceBspCollisionMaterialParametersCandidate m;m.name=string(in);auto flags=read<uint32_t>(in);if(flags>15)throw std::runtime_error("parameter flags");m.available=(flags&1)!=0;m.error=(flags&2)!=0;if(flags&4)m.primary=string(in);if(flags&8)m.secondary=string(in);params.push_back(std::move(m));}if(in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("trailing fixture");
 std::vector<SourceBspCollisionSurfaceEntryCandidate>entries;std::vector<uint32_t>mapping;std::vector<std::array<uint16_t,2>>pairs;
#ifdef NATIVE_BSP_SURFACE_TABLE
 AddVectoredExceptionHandler(1,fault);NativePrivateWorldImageFixture image(argv[4]);NativeVphysicsLibraryFixture physics(argv[5]);using Factory=void*(__cdecl*)(const char*,int*);auto factory=reinterpret_cast<Factory>(GetProcAddress(physics.module,"CreateInterface"));int status=-1;void*surface=factory("VPhysicsSurfaceProps001",&status);if(!surface||status)throw std::runtime_error("surface interface");auto surfaceTable=*reinterpret_cast<uintptr_t**>(surface);if(physics.rva(surfaceTable[1])!=0x16a50||physics.rva(surfaceTable[3])!=0x163b0)throw std::runtime_error("surface ABI");using Parse=int32_t(__thiscall*)(void*,const char*,const char*);for(const auto&s:scripts)reinterpret_cast<Parse>(surfaceTable[1])(surface,s.name.c_str(),s.text.c_str());
 systemTable[77]=reinterpret_cast<uintptr_t>(&findMaterial);materialTable[42]=reinterpret_cast<uintptr_t>(&isError);materialTable[11]=reinterpret_cast<uintptr_t>(&findVar);varTable[6]=reinterpret_cast<uintptr_t>(&getString);uintptr_t system=reinterpret_cast<uintptr_t>(systemTable.data());auto systemObject=&system;std::memcpy(image.base+0x69417c,&systemObject,4);std::memcpy(image.base+0x6377f8,&surface,4);materials.reserve(count);for(const auto&m:params)materials.push_back({materialTable.data(),&m,{{{varTable.data(),m.primary?&*m.primary:nullptr},{varTable.data(),m.secondary?&*m.secondary:nullptr}}}});
 struct Fragment{uint8_t*code;std::vector<uint8_t>bytes;};std::vector<Fragment>fragments;auto fragment=[&](size_t a,size_t z){size_t n=z-a;auto p=static_cast<uint8_t*>(VirtualAlloc(nullptr,n+1,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!p)throw std::runtime_error("fragment allocation");std::memcpy(p,image.base+a,n);p[n]=0xc3;fragments.push_back({p,std::vector<uint8_t>(p,p+n+1)});DWORD old;if(!VirtualProtect(p,n+1,PAGE_EXECUTE_READ,&old))throw std::runtime_error("fragment protection");FlushInstructionCache(GetCurrentProcess(),p,n+1);return p;};auto textures=fragment(0x14ab60,0x14ac2d),displacement=fragment(0x14c740,0x14c81d);
 const auto td=lump(2,32),ti=lump(6,72),strings=lump(43,1),names=lump(44,4),di=lump(26,176),fi=lump(7,56);if(td[1]!=count)throw std::runtime_error("native material count");std::vector<uint8_t>world(0x240,0xa9),runtime(count*8+32,0xb7);std::fill(runtime.begin()+16,runtime.end()-16,uint8_t(0));auto setPointer=[&](size_t at,const void*p){auto v=reinterpret_cast<uintptr_t>(p);std::memcpy(world.data()+at,&v,4);};setPointer(0x218,runtime.data()+16);setPointer(0x214,b.data()+strings[0]);std::memcpy(world.data()+0x210,&count,4);auto originalWorld=world;const auto originalMap=b;image.seal();invokeTextures(textures,world.data(),b.data()+td[0],b.data()+names[0],count);using Flags=void(__cdecl*)(void*,const void*,uint32_t);reinterpret_cast<Flags>(image.base+0x14ac60)(world.data(),b.data()+ti[0],uint32_t(ti[1]));
 for(size_t i=0;i<count;++i){uint32_t name;uint16_t prop,flags;std::memcpy(&name,runtime.data()+16+8*i,4);const char*p=reinterpret_cast<const char*>(name);if(p<reinterpret_cast<const char*>(b.data()+strings[0])||p>=reinterpret_cast<const char*>(b.data()+strings[0]+strings[1]))throw std::runtime_error("native surface name pointer");std::memcpy(&prop,runtime.data()+16+8*i+4,2);std::memcpy(&flags,runtime.data()+16+8*i+6,2);entries.push_back({p,prop,flags});}
 for(size_t i=0;i<ti[1];++i)mapping.push_back(word(ti[0]+72*i+68));std::vector<SourceDisplacementFaceInputCandidate>faces(fi[1]);std::memcpy(faces.data(),b.data()+fi[0],faces.size()*56);auto faceMap=sourceDisplacementFaceMapCandidate(faces,uint32_t(di[1]));for(auto face:faceMap){if(face==65535||face>=faces.size())throw std::runtime_error("native face binding");int16_t texture;std::memcpy(&texture,faces[face].data()+10,2);if(texture<0||size_t(texture)>=mapping.size()||mapping[size_t(texture)]>=count)throw std::runtime_error("native texture binding");std::array<uint8_t,0x114>record;record.fill(0xcc);std::fill(record.begin()+0x10,record.end()-0x10,uint8_t(0));const auto before=record;invokeDisplacement(displacement,world.data(),record.data()+0x10,mapping[size_t(texture)]);for(size_t i=0;i<record.size();++i)if((i<0x7c||i>=0x80)&&record[i]!=before[i])throw std::runtime_error("displacement write guard");std::array<uint16_t,2>pair;std::memcpy(pair.data(),record.data()+0x7c,4);pairs.push_back(pair);}
 if(b!=originalMap||world!=originalWorld)throw std::runtime_error("map/world changed");for(size_t i=0;i<16;++i)if(runtime[i]!=0xb7||runtime[runtime.size()-1-i]!=0xb7)throw std::runtime_error("surface write guard");for(const auto&f:fragments){if(std::memcmp(f.code,f.bytes.data(),f.bytes.size()))throw std::runtime_error("fragment changed");VirtualFree(f.code,0,MEM_RELEASE);}image.verify();physics.verify();VirtualFree(image.base,0,MEM_RELEASE);std::cout<<"native material requests="<<requests<<" findvars="<<findVars<<" stringgets="<<stringGets<<'\n';
#else
 (void)word;(void)lump;const SourceSurfaceNameTableCandidate properties(scripts);const auto before=b;const SourceBspCollisionSurfaceTableCandidate table(b,params,properties);entries=table.entries();mapping=table.textureToSurface();pairs=table.displacementProperties();if(b!=before)throw std::runtime_error("owned BSP changed");
#endif
 put(out,uint32_t(entries.size()));for(const auto&e:entries){putString(out,e.name);put(out,e.property);put(out,e.flags);}put(out,uint32_t(mapping.size()));for(auto id:mapping)put(out,id);put(out,uint32_t(pairs.size()));for(const auto&p:pairs){put(out,p[0]);put(out,p[1]);}out.flush();if(!out)throw std::runtime_error("surface output");std::cout<<"PASS surface table materials="<<entries.size()<<" texinfo="<<mapping.size()<<" displacements="<<pairs.size()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
