#pragma once
#include <string_view>
// Decimal scanner boundary only; exact native decimal-to-double conversion pending.
// Default C locale. Operator spelling and malformed-exponent rollback recovered.
inline bool expressionNumberSpan(std::string_view text,size_t&cursor){
 if(cursor>text.size())return false;
 size_t p=cursor;
 while(p<text.size()&&(text[p]==' '||text[p]=='\t'||text[p]=='\n'||text[p]=='\r'||text[p]=='\v'||text[p]=='\f'))++p;
 if(p<text.size()&&(text[p]=='+'||text[p]=='-'))++p;
 auto digit=[](char c){return c>='0'&&c<='9';};
 bool any=false;
 while(p<text.size()&&digit(text[p])){any=true;++p;}
 if(p<text.size()&&text[p]=='.'){
  ++p;while(p<text.size()&&digit(text[p])){any=true;++p;}
 }
 if(!any)return false;
 const size_t mantissaEnd=p;
 if(p<text.size()&&(text[p]=='e'||text[p]=='E'||text[p]=='d'||text[p]=='D')){
  ++p;if(p<text.size()&&(text[p]=='+'||text[p]=='-'))++p;
  const size_t exponentStart=p;
  while(p<text.size()&&digit(text[p]))++p;
  if(p==exponentStart)p=mantissaEnd;
 }
 cursor=p;return true;
}
