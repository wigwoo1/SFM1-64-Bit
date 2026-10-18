#pragma once
#include <string>
// DmeCurveInfo plus 12 typed descendants; parent getters and instance type checks recovered.
inline bool recoveredCurveInfoClass(const std::string&type){
 return type=="DmeCurveInfo"||type=="DmeIntCurveInfo"||type=="DmeFloatCurveInfo"||type=="DmeBoolCurveInfo"||type=="DmeColorCurveInfo"||type=="DmeVector2CurveInfo"||type=="DmeVector3CurveInfo"||type=="DmeVector4CurveInfo"||type=="DmeQAngleCurveInfo"||type=="DmeQuaternionCurveInfo"||type=="DmeVMatrixCurveInfo"||type=="DmeStringCurveInfo"||type=="DmeTimeCurveInfo";
}

