#pragma once
#include <cstdint>
// Native1036CD90 compares and stores packed four-byte color exactly.
template<class Permission,class Undo,class Notify,class Finish>bool writeRecoveredColorCandidate(uint32_t&storage,uint32_t incoming,Permission permission,Undo undo,Notify notify,Finish finish){if(storage==incoming||!permission())return false;undo();storage=incoming;notify();finish();return true;}
