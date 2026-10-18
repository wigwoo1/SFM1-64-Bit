#pragma once
#include <cstdint>
// 1028bee0. field80 is read twice; named-child adapter includes native typing.
template<class Adapter>
inline auto editorRateObjectCandidate(Adapter& a) -> decltype(a.resolve(a.editorHandle80())) {
    if(!a.resolve(a.editorHandle80()))return nullptr;
    auto* primary=a.resolve(a.editorHandle80());
    auto* settings=a.namedElement(primary,"settings");
    if(!settings)return nullptr;
    return a.namedElement(settings,"renderSettings");
}
