#pragma once
#include <string>
// Registered immediate DmeLog descendants proven by native parent getters.
inline bool recoveredLogClass(const std::string&type){
 return type=="DmeLog"||type=="DmeIntLog"||type=="DmeFloatLog"||type=="DmeBoolLog"||type=="DmeColorLog"||type=="DmeVector2Log"||type=="DmeVector3Log"||type=="DmeVector4Log"||type=="DmeQAngleLog"||type=="DmeQuaternionLog"||type=="DmeVMatrixLog"||type=="DmeStringLog"||type=="DmeTimeLog";
}
