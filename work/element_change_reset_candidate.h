#pragma once
// 1035dbd0: clear change-pending state and accumulated low two mask bits.
template<class Element,class Adapter>
inline void resetElementChangeCandidate(Element* e,Adapter& a) {
    a.setState(e,a.state(e)&~0x10u);
    a.setPacked(e,a.packed(e)&~3u);
}
