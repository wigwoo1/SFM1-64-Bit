#pragma once
#include <cstdint>
// Attempt phase1002a6cb..1002a77f only. Caller performs the earlier flag policy
// and supplies later failure UI. Output storage is retained across attempts.
template<class Backend,class Interface>auto studioD3D9CreationAttemptsWithOutputCandidate(Backend& backend,Interface* output){
 auto result=backend.create(output);
 if(static_cast<int32_t>(result)>=0&&output)return output;
 backend.wait(1000);
 result=backend.create(output);
 if(static_cast<int32_t>(result)>=0&&output)return output;
 if(backend.fallbackEnabled()){
  backend.configureFallback();
  result=backend.create(output);
 }
 if(static_cast<int32_t>(result)>=0&&output)return output;
 backend.failure();
 return static_cast<decltype(output)>(nullptr);
}
template<class Backend>auto studioD3D9CreationAttemptsCandidate(Backend& backend){
 return studioD3D9CreationAttemptsWithOutputCandidate(backend,backend.initialOutput());
}
