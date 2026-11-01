#include "texture_allocation_request_candidate.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
static void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
static uint32_t read(std::istream&i){uint32_t n;need(bool(i.read(reinterpret_cast<char*>(&n),4)),"word");return n;}
static std::string text(std::istream&i){const auto n=read(i);need(n&&n<=259,"text extent");std::string s(n,'\0');need(bool(i.read(s.data(),n)),"text");return s;}
static void text(std::ostream&o,const std::string&s){const auto n=uint32_t(s.size());o.write(reinterpret_cast<const char*>(&n),4);o.write(s.data(),n);}
static std::string trace(const TextureAllocationRequestCandidate&p){
    std::string s="D "+std::to_string(p.deviceValue)+"\n";
    if(p.batch){s+="CREATE";for(auto n:p.batch->arguments)s+=" "+std::to_string(n);s+="\nNAME "+p.batch->name+"\nGROUP "+p.batch->group+"\n";}
    if(p.depth){s+="DEPTH";for(auto n:p.depth->arguments)s+=" "+std::to_string(n);s+="\nNAME "+p.depth->name+"\n";}
    return s;
}
static int safety(){
    TextureAllocationDescriptorCandidate input{0,3,16,8,1,4,2,0,"fixture/allocation","Allocation group"};
    std::function<std::optional<uint32_t>()>device=[]()->std::optional<uint32_t>{return 1;};
    TextureAllocationRequestCandidate output;need(textureAllocationRequestCandidate(input,device,output),"initial plan");const auto before=trace(output);unsigned checks=0,calls=0;
    for(unsigned mode=0;mode<24;++mode){auto v=input;auto d=device;
        if(mode==0)d={};if(mode==1)d=[]()->std::optional<uint32_t>{return {};};if(mode==2)d=[]()->std::optional<uint32_t>{throw std::bad_alloc();};
        if(mode==3)v.width=0;if(mode==4)v.height=0;if(mode==5)v.depth=0;if(mode==6)v.mips=0;if(mode==7)v.frames=0;
        if(mode==8)v.width=65536;if(mode==9)v.height=65536;if(mode==10)v.depth=65536;if(mode==11)v.mips=65536;if(mode==12)v.frames=9;
        if(mode==13)v.format=64;if(mode==14){v.flags=0x10000;v.frames=1;}
        if(mode==15)v.name.clear();if(mode==16)v.name.assign(80,'a');if(mode==17)v.name="Upper";if(mode==18)v.name="a\\b";if(mode==19)v.name="a.vtf";
        if(mode==20)v.group.clear();if(mode==21)v.group.assign(256,'a');if(mode==22)v.name=std::string("a\0b",3);if(mode==23)v.group=std::string(1,'\x80');
        need(!textureAllocationRequestCandidate(v,d,output)&&trace(output)==before&&output.deviceActive&&output.batch&&!output.depth,"refusal preserves complete output");++checks;
    }
    device=[&]()->std::optional<uint32_t>{++calls;input.name="mutated";input.width=1;device={};return 1;};
    need(textureAllocationRequestCandidate(input,device,output)&&calls==1&&output.batch->name=="fixture/allocation"&&output.batch->arguments[1]==16,"descriptor/service snapshot");++checks;
    input.name="fixture/inactive";device=[]()->std::optional<uint32_t>{return 0x100;};need(textureAllocationRequestCandidate(input,device,output)&&!output.deviceActive&&!output.batch&&!output.depth&&output.deviceValue==0x100,"inactive replaces stale commands without success claim");++checks;
    std::cout<<"PASS owned_allocation_request_safety_checks="<<checks<<" real_GPU_allocations=0\n";return 0;
}
int main(int argc,char**argv){try{
    static_assert(sizeof(void*)==8,"owned x64 allocation request planner");if(argc==2&&std::string(argv[1])=="--test")return safety();
    need(argc==3&&!std::ifstream(argv[2]).good(),"queries fresh-output");std::ifstream in(argv[1],std::ios::binary);std::ofstream out(argv[2],std::ios::binary);need(bool(in)&&bool(out),"IO");const auto count=read(in);need(count&&count<=16384,"cases");
    for(uint32_t serial=0;serial<count;++serial){std::array<uint32_t,11>q{};for(auto&n:q)n=read(in);const auto name=text(in),group=text(in);need(q[10]<=2,"test handle mode");
        TextureAllocationDescriptorCandidate v{q[0],q[2],q[3],q[4],q[5],q[6],q[7],q[8],name,group};const auto before=q;
        TextureAllocationRequestCandidate p;uint32_t calls=0;need(textureAllocationRequestCandidate(v,[&]()->std::optional<uint32_t>{++calls;return q[9];},p)&&calls==1,"request plan");need(q==before&&v.name==name&&v.group==group,"inputs preserved");
        // Test-only reconstruction of the native controlled-service observations.
        // These synthetic IDs/status are absent from the production helper.
        std::vector<uint32_t>handles(v.frames,0);
        if(p.batch)for(uint32_t i=0;i<p.batch->arguments[0];++i)if(q[10]==1||(q[10]==2&&i==0))handles[i]=0x4000u+i;
        if(p.depth){need(p.depth->destinationFrame<handles.size(),"depth frame extent");handles[p.depth->destinationFrame]=q[10]==1?0x5000u:0;}
        const std::array<uint32_t,5>record{serial,uint32_t(p.deviceActive),q[1]|(p.deviceActive?2u:0u),uint32_t(bool(p.batch)),uint32_t(bool(p.depth))};out.write(reinterpret_cast<const char*>(record.data()),sizeof(record));out.write(reinterpret_cast<const char*>(handles.data()),std::streamsize(handles.size()*4));text(out,trace(p));
    }
    need(in.peek()==EOF&&in.eof(),"query framing");out.flush();need(bool(out),"trace write");std::cout<<"PASS owned_allocation_request_cases="<<count<<" commands=PLANNED simulated_handle_ids=1 real_GPU_allocations=0\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
