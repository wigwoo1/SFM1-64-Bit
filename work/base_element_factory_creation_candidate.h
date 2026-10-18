#pragma once
#include <cstdint>
// 103621b0: original allocation is 0x5c x86 bytes. Adapter allocates its own
// native object layout; caller arguments remain opaque until identities proven.
struct BaseElementFactoryArgumentsCandidate {
    std::uintptr_t arg0,arg1,arg2,arg3,arg4;
};
template<class Adapter>
inline auto createBaseElementCandidate(const BaseElementFactoryArgumentsCandidate& args,Adapter& a)
    -> decltype(a.allocateNativeBaseElement()) {
    auto* storage=a.allocateNativeBaseElement();
    if(!storage)return nullptr;
    const auto name=a.factoryName();
    return a.constructBaseElement(storage,args.arg0,name,args.arg4,args.arg2,args.arg3);
}
