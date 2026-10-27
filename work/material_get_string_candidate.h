#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <optional>
#include <cstdarg>
#include <cstdio>
#include <stdexcept>
#include <cmath>
#include <algorithm>
#include <locale.h>
// Owned immediate snapshot of CMaterialVar GetString, not its borrowed pointer.
// Windows UCRT per-call compatibility formatting; global locale/options unchanged.
struct MaterialGetStringInputCandidate{
 uint32_t type=6,components=4,integer=0;
 std::array<uint32_t,16>numbers{};
 std::string text;
 std::optional<std::string>resourceName;
 bool environmentTexture=false;
};
inline std::string materialGetStringFormatCandidate(const char*format,...){
 struct Locale{_locale_t value=_create_locale(LC_ALL,"C");~Locale(){if(value)_free_locale(value);}};static const Locale locale;
 if(!locale.value)throw std::bad_alloc();std::array<char,1024>buffer{};va_list args;va_start(args,format);
 int n=__stdio_common_vsprintf(_CRT_INTERNAL_PRINTF_LEGACY_MSVCRT_COMPATIBILITY|_CRT_INTERNAL_PRINTF_LEGACY_THREE_DIGIT_EXPONENTS,buffer.data(),buffer.size(),format,locale.value,args);va_end(args);
 if(n<0||size_t(n)>=buffer.size())throw std::runtime_error("material text formatting extent");return std::string(buffer.data(),size_t(n));
}
inline std::string materialGetStringFloatCandidate(uint32_t bits,unsigned decimals){
 float value;std::memcpy(&value,&bits,4);const double wide=value;
 if(std::isfinite(wide)&&std::fabs(wide)>=1e17){
  // The installed CRT emits at most17 significant digits and zero-fills the
  // remaining integral places. UCRT's %f otherwise prints the exact huge integer.
  const auto scientific=materialGetStringFormatCandidate("%.16e",wide);const auto marker=scientific.find('e');
  if(marker==std::string::npos)throw std::runtime_error("material scientific conversion");
  std::string digits=scientific.substr(0,marker);digits.erase(std::remove(digits.begin(),digits.end(),'.'),digits.end());
  const int exponent=std::stoi(scientific.substr(marker+1));if(exponent<16||exponent>38)throw std::runtime_error("material float exponent extent");
  digits.append(size_t(exponent-16),'0');digits+='.';digits.append(decimals,'0');return digits;
 }
 return materialGetStringFormatCandidate(decimals==3?"%.3f":"%f",wide);
}
inline bool materialGetStringCandidate(const MaterialGetStringInputCandidate&input,std::string&output){
 if(input.type>15||input.components>4||input.text.size()>4095||(input.resourceName&&input.resourceName->size()>511))return false;
 std::string result;
 switch(input.type){
 case 0:result=materialGetStringFloatCandidate(input.numbers[0],6);break;
 case 1:result=input.text.substr(0,input.text.find('\0'));break;
 case 2:result="[ ";for(unsigned i=0;i<input.components;++i)result+=materialGetStringFloatCandidate(input.numbers[i],6)+" ";result+="]";break;
 case 3:if(input.environmentTexture){result="env_cubemap";break;}[[fallthrough]];
 case 8:if(!input.resourceName)return false;result=input.resourceName->substr(0,input.resourceName->find('\0'));break;
 case 4:{int32_t value;std::memcpy(&value,&input.integer,4);result=materialGetStringFormatCandidate("%d",value);break;}
 case 6:result="<UNDEFINED>";break;
 case 7:result="[ ";for(unsigned row=0;row<4;++row)for(unsigned column=0;column<4;++column)result+=materialGetStringFloatCandidate(input.numbers[column*4+row],3)+" ";result+="]";if(result.size()>511)result.resize(511);break;
 default:break; // Original warning + shared empty literal; warning owned by caller.
 }
 output.swap(result);return true;
}
