#pragma once
#include <algorithm>
#include <cstdint>

namespace source_sfm_collision_detail {
// Values in the installed SFM client. TF-specific values belong to that build.
enum Group : int32_t {
    none=0, debris=1, debrisTrigger=2, interactiveDebris=3, interactive=4, player=5,
    breakableGlass=6, vehicle=7, playerMovement=8, npc=9, inVehicle=10,
    weapon=11, vehicleClip=12, projectile=13, doorBlocker=14, passableDoor=15,
    dissolving=16, pushaway=17, debrisProjectileBlocker=21,
    grenades=22, combatObject=25, rockets=26, respawnRoom=27
};

inline bool baseRules(int32_t low,int32_t high){
    if(high==pushaway){
        if(low==player||low==playerMovement)return false;
        if(low==debris)return true;
    }
    if(low==projectile&&high==debrisProjectileBlocker)return true;
    if(low==debrisProjectileBlocker||high==debrisProjectileBlocker||
       low==inVehicle||high==inVehicle)return false;
    if(high==doorBlocker&&low!=npc)return false;
    if(low==player&&high==passableDoor)return false;
    if(low==debris||low==debrisTrigger)return false;
    if((low==dissolving||high==dissolving)&&low!=none)return false;
    if(low==interactiveDebris&&(high==interactiveDebris||high==playerMovement||high==player))return false;
    if(low==breakableGlass&&high==breakableGlass)return false;
    if(high==interactive&&low!=none)return false;
    if(high==projectile&&(low==weapon||low==projectile))return false;
    if(high==weapon&&(low==vehicle||low==player||low==npc))return false;
    if(low==vehicleClip||high==vehicleClip)return low==vehicle;
    return true;
}
}

inline bool sourceSfmBaseCollisionGroupsCandidate(int32_t first,int32_t second){
    const auto [low,high]=std::minmax(first,second);
    return source_sfm_collision_detail::baseRules(low,high);
}

inline bool sourceSfmTfCollisionGroupsCandidate(int32_t first,int32_t second){
    using namespace source_sfm_collision_detail;
    const auto [low,high]=std::minmax(first,second);
    if(low==playerMovement&&(high==weapon||high==projectile||high==rockets))return false;
    if(high==rockets){
        if(low==player)return true;
        if(low==weapon||low==grenades)return false;
    }
    if(high==grenades&&(low==player||low==playerMovement||low==grenades))return false;
    if(high==respawnRoom)return low==player||low==playerMovement;
    if(high==combatObject&&(low==player||low==playerMovement))return false;
    return baseRules(low,high);
}

// C_BaseEntity's default ShouldCollide virtual. Specialized entities may
// supply a different rule before the group check.
inline bool sourceSfmBaseEntityCollisionCandidate(int32_t entityGroup,uint32_t mask){
    constexpr uint32_t contentsDebris=0x04000000u;
    return entityGroup!=source_sfm_collision_detail::debris||(mask&contentsDebris)!=0;
}
