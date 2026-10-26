#pragma once
#include "source_material_patch_policy_candidate.h"
#include "material_decimal_prefix_candidate.h"
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <limits>
// Raw material KeyValues, before filesystem directives, conditional evaluation,
// shader processing or patch/include resolution. Uses the existing owned node
// carrier; kind7 stores a uint64 VALUE in its uint64 pointer field, never an
// address. The separately verified patch-only validator still rejects kind7.
struct SourceMaterialKeyValuesDocumentCandidate {
 std::vector<SourceMaterialPatchNodeCandidate>roots;
 bool numericRangeError=false;
};
class SourceMaterialKeyValuesParserCandidate {
 const std::string&source;size_t position=0,count=0;bool rangeError=false;
 struct Token{enum Kind{End,Text,Open,Close,Equal}kind=End;std::string text;bool quoted=false;};
 Token next(){
  for(;;){while(position<source.size()&&(source[position]==' '||source[position]=='\t'||source[position]=='\r'||source[position]=='\n'||source[position]=='\f'||source[position]=='\v'))++position;if(position+1<source.size()&&source[position]=='/'&&source[position+1]=='/'){while(position<source.size()&&source[position]!='\n')++position;continue;}break;}
  if(position==source.size())return {};char c=source[position++];if(c=='{')return {Token::Open,{}};if(c=='}')return {Token::Close,{}};if(c=='=')return {Token::Equal,{}};
  Token token;token.kind=Token::Text;if(c=='"'){token.quoted=true;bool closed=false;while(position<source.size()){char x=source[position++];if(x=='"'){closed=true;break;}token.text+=x;if(token.text.size()>32767)throw std::runtime_error("material KeyValues token extent");}if(!closed)throw std::runtime_error("unterminated material KeyValues string");}
  else{token.text+=c;while(position<source.size()){char x=source[position];if(x==' '||x=='\t'||x=='\r'||x=='\n'||x=='\f'||x=='\v'||x=='{'||x=='}'||x=='='||x=='"')break;token.text+=x;++position;if(token.text.size()>32767)throw std::runtime_error("material KeyValues token extent");}}
  // A raw reader must not silently turn directives/conditions into material
  // parameters. Those require the verified filesystem/evaluation layer.
  if(!token.quoted&&!token.text.empty()&&token.text[0]=='[')throw std::runtime_error("material KeyValues condition resolver required");return token;
 }
 void scalar(SourceMaterialPatchNodeCandidate&node,const std::string&value){
  node.kind=1;node.text=value;if(value.empty())return;
  if(value.size()==18&&value[0]=='0'&&value[1]=='x'){uint64_t result=0;for(size_t i=2;i<18;++i){unsigned char c=static_cast<unsigned char>(value[i]);uint64_t nibble;if(c>='0'&&c<='9')nibble=c-'0';else if(c>='a'&&c<='f')nibble=c-'a'+10;else if(c>='A'&&c<='F')nibble=c-'A'+10;else throw std::runtime_error("invalid material KeyValues uint64 hexadecimal");result=(result<<4)|nibble;}node.kind=7;node.pointer=result;node.text.clear();return;}
  // This original32 CRT uses decimal strtol and pre-C99 strtod. Modern CRT
  // hex-float/NaN/Inf acceptance must not broaden the file grammar silently.
  struct RestoreErrno{int old=errno;~RestoreErrno(){errno=old;}}restore;
  errno=rangeError?ERANGE:0;char*integerEnd=nullptr;long integer=std::strtol(value.c_str(),&integerEnd,10);rangeError=errno==ERANGE;
  static_assert(sizeof(long)==4,"material parser requires32bit C long semantics");
  const char*begin=value.c_str();
  // Reuse the verified legacy conversion, including D exponents and its
  // decimal rounding. A modern strtod has a different grammar and arithmetic.
  const auto parsed=materialDecimalPrefixCandidate(value);
  const char*floatEnd=begin+parsed.consumed;const double number=parsed.value;
  // The original strtod wrapper sets sticky ERANGE on double overflow or
  // nonzero input converted to double zero, not on a nonzero subnormal or
  // a later float32 underflow. Literal signed zero remains signed.
  rangeError=rangeError||parsed.rangeError;
  const bool overflow=(integer==LONG_MAX||integer==LONG_MIN)&&rangeError;const auto end=begin+value.size();
  if(floatEnd>integerEnd&&floatEnd==end){float v=static_cast<float>(number);node.kind=3;std::memcpy(&node.bits,&v,4);node.text.clear();}
  else if(integerEnd==end&&!overflow){node.kind=2;node.bits=uint32_t(integer);node.text.clear();}
 }
 std::vector<SourceMaterialPatchNodeCandidate>body(size_t depth){
  if(depth>16)throw std::runtime_error("material KeyValues nesting extent");std::vector<SourceMaterialPatchNodeCandidate>nodes;
  for(;;){auto key=next();if(key.kind==Token::Close)return nodes;if(key.kind!=Token::Text||key.text.empty())throw std::runtime_error("material KeyValues nonempty key/closing brace required");if(++count>65536)throw std::runtime_error("material KeyValues node extent");auto value=next();if(value.kind==Token::Equal)throw std::runtime_error("material KeyValues replacement assignment resolver required");SourceMaterialPatchNodeCandidate node;node.key=std::move(key.text);if(value.kind==Token::Open)node.children=body(depth+1);else if(value.kind==Token::Text)scalar(node,value.text);else throw std::runtime_error("material KeyValues value required");nodes.push_back(std::move(node));}
 }
public:
 explicit SourceMaterialKeyValuesParserCandidate(const std::string&s,bool initialRangeError=false):source(s),rangeError(initialRangeError){if(source.size()>(1u<<24))throw std::runtime_error("material KeyValues source extent");for(unsigned char c:source)if((c<32&&c!='\t'&&c!='\r'&&c!='\n'&&c!='\f'&&c!='\v')||c>126)throw std::runtime_error("material KeyValues source encoding");}
 SourceMaterialKeyValuesDocumentCandidate parse(){SourceMaterialKeyValuesDocumentCandidate result;for(;;){auto key=next();if(key.kind==Token::End)break;if(key.kind==Token::Text&&(sourceMaterialPatchLowerCandidate(key.text)=="#base"||sourceMaterialPatchLowerCandidate(key.text)=="#include"))throw std::runtime_error("material KeyValues file directive resolver required");if(key.kind!=Token::Text||key.text.empty()||next().kind!=Token::Open)throw std::runtime_error("material KeyValues root block required");SourceMaterialPatchNodeCandidate node;node.key=std::move(key.text);if(++count>65536)throw std::runtime_error("material KeyValues node extent");node.children=body(0);result.roots.push_back(std::move(node));}if(result.roots.empty())throw std::runtime_error("empty material KeyValues source");result.numericRangeError=rangeError;return result;}
};
inline SourceMaterialKeyValuesDocumentCandidate sourceMaterialKeyValuesParseCandidate(const std::string&source,bool initialRangeError=false){return SourceMaterialKeyValuesParserCandidate(source,initialRangeError).parse();}
