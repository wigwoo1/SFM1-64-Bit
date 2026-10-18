#pragma once
#include "float_component_write_sequence.h"
#include <vector>
// ifm 1036EF50: bounds/equality precede permission; callbacks bracket x87 store.
// Undo callback supplies policy/capture; native undo allocation is not implemented here.
template<class Permission,class Undo,class Before,class After,class Finish>
bool writeRecoveredFloatArrayElementCandidate(uint32_t& storage,int32_t index,
 uint32_t incoming,Permission permission,Undo undo,Before before,After after,Finish finish){
 float oldValue,newValue;std::memcpy(&oldValue,&storage,4);
 std::memcpy(&newValue,&incoming,4);
 if(!floatMutationNeeded(oldValue,newValue)||!permission())return false;
 undo();before(index,index);
 storage=recoveredX87FloatStoreBitsCandidate(incoming);
 after(index,index,1);finish();return true;
}

// Bounds stay in the array adapter; element storage may also be a wire scratch word.
template<class Permission,class Undo,class Before,class After,class Finish>
bool writeRecoveredFloatArrayCandidate(std::vector<uint32_t>& storage,int32_t index,
 uint32_t incoming,Permission permission,Undo undo,Before before,After after,Finish finish){
 if(index<0||size_t(index)>=storage.size())return false;
 return writeRecoveredFloatArrayElementCandidate(storage[size_t(index)],index,incoming,permission,undo,before,after,finish);
}
