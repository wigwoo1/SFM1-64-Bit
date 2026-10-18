#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
// Collision object identity excludes the original object's vtable/refcount
// prefix. It is an owned token, not original pointer+8 or a truncated address.
struct SourceStudioVCollideRecordCandidate {
    uint16_t flags=0;
    uint64_t collisionObjectId=0;
};
// Original datacache1000EB80 type2, unsuccessful-read branch, diagnostics off.
// Begin-load/IO dispatch is separate. Failure does NOT itself clear old data.
inline bool sourceStudioVCollideFailedCompletionCandidate(SourceStudioVCollideRecordCandidate&r){
    r.flags|=2;return false;
}
// Original1000CEB0 ->1000DF90 READY-record scope. Native load scheduling is
// intentionally NOT approximated. Unknown/in-flight records are rejected even
// when no object is currently present. No missing-file -> cache-absence rule.
// Services supplies a virtual-model group's handles (including unused group0),
// lookup of already-cached group records, and ownership retention. Lifetimes,
// synchronization and actual IO belong to that provider, not this policy.
template<class Services>
uint64_t sourceStudioVCollideReadyQueryCandidate(SourceStudioVCollideRecordCandidate&r,
    uint16_t handle,bool disabled,Services&services){
    if(disabled||handle==0xffff)return 0;
    if(!(r.flags&2))throw std::runtime_error("VCollide load not completed; native scheduling required");
    if(!(r.flags&0x80)){
        r.flags|=0x80;
        if(!r.collisionObjectId){
            const auto groups=services.virtualModelGroups(handle);
            for(std::size_t i=1;i<groups.size();++i){
                const auto id=services.cachedGroupObject(groups[i]);
                if(id){r.collisionObjectId=id;services.retainCollisionObject(id);break;}
            }
        }
    }
    return r.collisionObjectId;
}
