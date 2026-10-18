#pragma once
inline bool studioInterfaceNameEqualCandidate(const char* left,const char* right){
 for(;;++left,++right){
  unsigned a=static_cast<unsigned char>(*left),b=static_cast<unsigned char>(*right);
  if(a>='A'&&a<='Z')a+=32;
  if(b>='A'&&b<='Z')b+=32;
  if(a!=b)return false;
  if(a==0)return true;
 }
}
// Services return live pointers; recognized names retain status0 even if pointer null.
template<class Services>void* studioShaderInterfaceFactoryCandidate(const char* name,int* status,Services& services){
 if(status)*status=0;
 if(studioInterfaceNameEqualCandidate(name,"ShaderDevice002"))return services.shaderDevice();
 if(studioInterfaceNameEqualCandidate(name,"ShaderApi030"))return services.shaderApi();
 if(studioInterfaceNameEqualCandidate(name,"ShaderShadow010"))return services.shaderShadow();
 if(status)*status=1;
 return nullptr;
}
