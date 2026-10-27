#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
// CPU service boundary only. Tokens are borrowed provider results, not owned
// proxy instances or permission to omit a material declaration.
struct MaterialProxyFactoryRouteServicesCandidate {
 std::function<std::optional<uintptr_t>(const std::string&)>client;
 std::function<std::optional<bool>()>toolsEnabled;
 std::function<std::optional<int32_t>()>toolCount;
 std::function<std::optional<uintptr_t>(int32_t,const std::string&)>tool;
};
struct MaterialProxyFactoryRouteResultCandidate {uintptr_t token=0;bool warning=false;};
// A present result with token0 is a definite miss from ALL consulted services.
// nullopt is an unavailable/unverified service, not a native registry miss.
// Preserve original client -> enabled query (even on client hit) -> tool order.
inline std::optional<MaterialProxyFactoryRouteResultCandidate>materialProxyFactoryRouteCandidate(
 const std::string&name,uint8_t suppressWarning,const MaterialProxyFactoryRouteServicesCandidate&services)noexcept{
 try{
  if(name.size()>255||name.find('\0')!=std::string::npos||!services.client||!services.toolsEnabled)return {};
  for(unsigned char c:name)if(c>=128)return {}; // Native non-ASCII/locale lookup is outside this gate.
  auto client=services.client(name);if(!client)return {};auto enabled=services.toolsEnabled();if(!enabled)return {};
  auto token=*client;
  if(*enabled&&!token){
   if(!services.toolCount)return {};auto count=services.toolCount();if(!count||*count>4096||(*count>0&&!services.tool))return {};
   for(int32_t i=0;i<*count;++i){auto result=services.tool(i,name);if(!result)return {};if(*result){token=*result;break;}}
  }
  return MaterialProxyFactoryRouteResultCandidate{token,!token&&!suppressWarning};
 }catch(...){return {};}
}
