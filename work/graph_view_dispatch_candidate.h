#pragma once
#include <cstdint>
template<class Adapter>void graphViewStaticDispatchCandidate(int kind,std::uint32_t id,void** argv,Adapter& a){
 if(kind!=0 || id>3)return;
 switch(id){
 case 0:a.contextMenuSlot50(argv[1]);break;
 case 1:a.escapeSlot51();break;
 default:a.keyEvent100d3ef0(*static_cast<void**>(argv[1]));break;
 }
}
template<class Adapter>int graphViewMetacallCandidate(int kind,int id,void** argv,Adapter& a){
 auto local=a.inheritedMetacall(kind,id,argv);
 if(local<0)return local;
 if(kind==0){if(local<4)graphViewStaticDispatchCandidate(kind,static_cast<std::uint32_t>(local),argv,a);local-=4;}
 return local;
}
