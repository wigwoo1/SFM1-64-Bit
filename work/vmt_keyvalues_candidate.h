#pragma once
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
struct VmtNodeCandidate {std::string key,value;std::vector<VmtNodeCandidate>children;bool block=false;};
// Syntax-preserving basic KeyValues candidate. Escape sequences disabled:
// backslashes in quoted material paths are literal. Includes, patches and
// platform conditions are not evaluated by this parser.
class VmtParserCandidate {
 const std::string&source;size_t position=0;
 struct Token {std::string text;enum Kind{Text,Open,Close,End}kind=End;};
 Token next(){
  for(;;){while(position<source.size()&&std::isspace(static_cast<unsigned char>(source[position])))++position;if(position+1<source.size()&&source[position]=='/'&&source[position+1]=='/'){position+=2;while(position<source.size()&&source[position]!='\n')++position;continue;}break;}
  if(position==source.size())return {};
  char c=source[position++];if(c=='\0')throw std::runtime_error("NUL in VMT");if(c=='{')return {"",Token::Open};if(c=='}')return {"",Token::Close};
  std::string text;if(c=='"'){bool closed=false;while(position<source.size()){char v=source[position++];if(v=='"'){closed=true;break;}if(v=='\0')throw std::runtime_error("NUL in VMT string");text+=v;}if(!closed)throw std::runtime_error("unterminated VMT quote");}else{text+=c;while(position<source.size()){char v=source[position];if(std::isspace(static_cast<unsigned char>(v))||v=='{'||v=='}'||v=='"')break;if(v=='\0')throw std::runtime_error("NUL in VMT token");text+=v;++position;}}
  return {text,Token::Text};
 }
 std::vector<VmtNodeCandidate> body(size_t depth){if(depth>64)throw std::runtime_error("VMT nesting limit");std::vector<VmtNodeCandidate>nodes;for(;;){auto key=next();if(key.kind==Token::Close)return nodes;if(key.kind!=Token::Text)throw std::runtime_error("VMT key/closing brace required");auto value=next();VmtNodeCandidate node;node.key=std::move(key.text);if(value.kind==Token::Open){node.block=true;node.children=body(depth+1);}else if(value.kind==Token::Text)node.value=std::move(value.text);else throw std::runtime_error("VMT value required");nodes.push_back(std::move(node));}}
public:
 explicit VmtParserCandidate(const std::string&s):source(s){}
 VmtNodeCandidate parse(){auto shader=next();if(shader.kind!=Token::Text||shader.text.empty())throw std::runtime_error("VMT shader required");if(next().kind!=Token::Open)throw std::runtime_error("VMT root block required");VmtNodeCandidate root;root.key=std::move(shader.text);root.block=true;root.children=body(0);if(next().kind!=Token::End)throw std::runtime_error("VMT trailing tokens/conditions unsupported");return root;}
};
inline VmtNodeCandidate parseVmtCandidate(const std::string&source){return VmtParserCandidate(source).parse();}
inline std::string vmtAsciiLowerCandidate(std::string text){for(char&c:text)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return text;}
inline std::vector<std::string> vmtValuesCandidate(const VmtNodeCandidate&node,const std::string&key){std::vector<std::string>result;auto target=vmtAsciiLowerCandidate(key);for(auto&child:node.children)if(!child.block&&vmtAsciiLowerCandidate(child.key)==target)result.push_back(child.value);return result;}
