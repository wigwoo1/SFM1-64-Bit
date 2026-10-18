from pathlib import Path
s=Path('work/probe_texture_frame.cpp').read_text().replace('projected_light_texture_frame_candidate.h','material_animation_frame_candidate.h')
s=s.replace('#include <array>', '#include <array>\n#include <string>')
a=s.index('#ifdef _WIN64');b=s.index('#else',a)
s=s[:a]+'''#ifdef _WIN64
 std::ifstream input(argv[1],std::ios::binary);unsigned cases=0;std::array<uint32_t,7>r{};
 while(input.read(reinterpret_cast<char*>(r.data()),sizeof(r))){auto v=materialAnimationFrameCandidate(r[0],r[1],r[2],r[3],int32_t(r[4]),true);if(uint32_t(v.frame)!=r[5]||uint32_t(v.previousFrame)!=r[6])throw std::runtime_error("material arithmetic mismatch case "+std::to_string(cases));++cases;}
 if(input.gcount()||!input.eof()||cases!=65536)throw std::runtime_error("oracle length");std::cout<<"x64_material_frames_exact=65536\\n";
'''+s[b:]
s=s.replace(r'game\bin\tools\ifm.dll',r'game\tf\bin\client.dll')
a=s.index(' // Copy only');b=s.index('#endif',a)
s=s[:a]+''' const size_t bytes=0x1000f1b0-0x1000f16a;auto copied=VirtualAlloc(nullptr,bytes+5,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);if(!copied)throw std::runtime_error("stub allocation");
 std::unique_ptr<void,void(*)(void*)> stubOwner(copied,[](void*p){VirtualFree(p,0,MEM_RELEASE);});std::memcpy(copied,reinterpret_cast<const void*>(0x1000f16a),bytes);
 const uint8_t tail[]={0x8b,0xc3,0x5b,0xc3};std::memcpy(static_cast<uint8_t*>(copied)+bytes,tail,sizeof(tail));
 if(!VirtualProtect(copied,bytes+sizeof(tail),PAGE_EXECUTE_READ,&oldProtect))throw std::runtime_error("stub protection");FlushInstructionCache(GetCurrentProcess(),copied,bytes+sizeof(tail));
 std::ofstream output(argv[1],std::ios::binary);uint32_t seed=0x98765432;const uint32_t bounds[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,0x7fc01234,0x7f801234};
 for(unsigned i=0;i<65536;++i){std::array<uint32_t,7>r{};for(unsigned j=0;j<4;++j){seed=seed*1664525u+1013904223u;r[j]=i<64?bounds[(i+j)%8]:seed;}r[4]=1+i%127;
 std::array<uint32_t,8>object{},clock{};object[3]=r[3];clock[3]=r[0];clock[4]=r[1];auto beforeObject=object,beforeClock=clock;uint32_t start=r[2],out[2]{};executeMaterialArithmetic(copied,object.data(),clock.data(),&start,int32_t(r[4]),out);r[5]=out[0];r[6]=out[1];if(object!=beforeObject||clock!=beforeClock||start!=r[2])throw std::runtime_error("input mutation");output.write(reinterpret_cast<const char*>(r.data()),sizeof(r));}
 output.flush();if(!output)throw std::runtime_error("oracle write");std::cout<<"original_x86_material_frames=65536\\n";
'''+s[b:]
needle='int main(int argc,char**argv)'
wrapper='''#ifndef _WIN64
__declspec(naked) void executeMaterialArithmetic(void*,uint32_t*,uint32_t*,uint32_t*,int32_t,uint32_t*){
 __asm {push ebp
 push esi
 push edi
 push ebx
 mov esi,[esp+24]
 mov eax,[esp+28]
 mov ebp,[esp+32]
 add ebp,4
 mov edi,[esp+36]
 call dword ptr [esp+20]
 mov ecx,[esp+40]
 mov [ecx],eax
 mov [ecx+4],edx
 pop ebx
 pop edi
 pop esi
 pop ebp
 ret}
}
#endif
'''
s=s.replace(needle,wrapper+needle)
Path('work/probe_material_animation_arithmetic.cpp').write_text(s)
Path('work/build_material_animation_arithmetic.cmd').write_text(Path('work/build_texture_frame.cmd').read_text().replace('probe_texture_frame','probe_material_animation_arithmetic'))
