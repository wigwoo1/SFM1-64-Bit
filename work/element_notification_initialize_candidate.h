#pragma once
// Notification-field subset of 10361c40, preserving original untouched bits.
// This is not a replacement for the complete constructor and its services.
template<class Element,class Adapter>
inline void initializeElementNotificationCandidate(Element* e,Adapter& a) {
    a.setState(e,a.state(e)&0xe0u);
    a.setPacked(e,a.packed(e)&~0xfu);
    a.clearNotificationHeads(e);
}
