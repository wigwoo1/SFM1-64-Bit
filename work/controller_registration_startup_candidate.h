#pragma once
#include <cstdint>
// Address tags select recovered helpers; adapter must supply native implementations.
template<class Adapter>void registerControllerStartupTablesCandidate(Adapter& a){
 constexpr std::uint32_t order[]={0x10476b10u,0x104769e0u,0x104769b0u,0x10476980u,0x10476950u,0x10476920u,0x104768f0u,0x104768c0u,0x10476890u,0x10476860u,0x10476830u,0x10476800u,0x104767d0u,0x104767a0u,0x10476770u,0x10476740u,0x10476710u,0x104766e0u,0x104766b0u,0x10476680u,0x10476650u,0x10476620u,0x104765f0u,0x104765c0u,0x10476590u,0x10476560u};
 for(auto tag:order)a.invokeRegistrationHelper(tag);
 if(a.lookupRegistration(a.firstTableCode()))return;
 a.registerTable(0,a.firstTable());a.registerTable(0,a.secondTable());
}
