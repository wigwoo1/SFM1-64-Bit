#pragma once
#include "source_material_keyvalues_parser_candidate.h"
#include <functional>
#include <optional>
// Logical GAME paths and a caller-supplied, read-only source provider. This
// resolves KeyValues #include/#base, NOT a material patch's plain include key,
// shader defaults, mounted filesystem priority, or effective IMaterial values.
inline std::string sourceMaterialKeyValuesPathCandidate(std::string path){
 if(path.empty()||path.size()>511||path[0]=='/'||path[0]=='\\')throw std::runtime_error("material logical path extent/root");std::vector<std::string>parts;std::string part;
 auto finish=[&]{if(part.empty()||part=="."){part.clear();return;}if(part==".."){if(parts.empty())throw std::runtime_error("material logical path escapes root");parts.pop_back();}else parts.push_back(part);part.clear();};
 for(unsigned char c:path){if(c<32||c>126||c==':'||c=='|'||c=='*'||c=='?'||c=='"'||c=='<'||c=='>')throw std::runtime_error("material logical path encoding");if(c=='/'||c=='\\')finish();else part+=char(c);}finish();if(parts.empty())throw std::runtime_error("empty material logical path");std::string result;for(const auto&p:parts){if(!result.empty())result+='/';result+=p;}return sourceMaterialPatchLowerCandidate(result);
}
struct SourceMaterialKeyValuesFilesResultCandidate {
 SourceMaterialKeyValuesDocumentCandidate document;
 std::vector<std::string>dependencies; // Ordered reads, including repeated files.
 bool retainsExistingPeers=false; // Original reuse keeps peers if only one local root was parsed.
};
using SourceMaterialKeyValuesSourceCandidate=std::function<std::optional<std::string>(const std::string&)>;
class SourceMaterialKeyValuesFilesCandidate {
 SourceMaterialKeyValuesSourceCandidate provider;std::vector<std::string>active,dependencies;size_t bytes=0,files=0,nodes=0;bool rangeError=false;
 struct Token{enum Kind{End,Text,Open,Close,Equal}kind=End;std::string text;size_t begin=0,end=0;};
 class TopLevel {
  const std::string&s;size_t at=0;
  static bool space(char c){return c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\v'||c=='\f';}
 public:
  explicit TopLevel(const std::string&source):s(source){}
  Token next(){for(;;){while(at<s.size()&&space(s[at]))++at;if(at+1<s.size()&&s[at]=='/'&&s[at+1]=='/'){while(at<s.size()&&s[at]!='\n')++at;}else break;}Token t;t.begin=at;if(at==s.size())return t;char c=s[at++];t.kind=Token::Text;if(c=='{')t.kind=Token::Open;else if(c=='}')t.kind=Token::Close;else if(c=='=')t.kind=Token::Equal;else if(c=='"'){bool closed=false;while(at<s.size()){char v=s[at++];if(v=='"'){closed=true;break;}t.text+=v;if(t.text.size()>32767)throw std::runtime_error("material directive token extent");}if(!closed)throw std::runtime_error("material directive unterminated quote");}else{t.text+=c;while(at<s.size()&&!space(s[at])&&s[at]!='{'&&s[at]!='}'&&s[at]!='='&&s[at]!='"'){t.text+=s[at++];if(t.text.size()>32767)throw std::runtime_error("material directive token extent");}}t.end=at;return t;}
 };
 static size_t count(const SourceMaterialPatchNodeCandidate&n,size_t depth=0){if(depth>17)throw std::runtime_error("resolved material node depth");size_t total=1;for(const auto&c:n.children){total+=count(c,depth+1);if(total>65536)throw std::runtime_error("resolved material node extent");}return total;}
 static void mergeBase(SourceMaterialPatchNodeCandidate&target,const SourceMaterialPatchNodeCandidate&base){
  // Original100712A0 uses strcmp, not path-oriented/case-folded FindKey.
  // Values/types survive a match; even a scalar can gain retained children.
  for(const auto&child:base.children){auto found=std::find_if(target.children.begin(),target.children.end(),[&](const auto&candidate){return candidate.key==child.key;});if(found==target.children.end())target.children.push_back(child);else mergeBase(*found,child);}
 }
 SourceMaterialKeyValuesDocumentCandidate load(const std::string&path,bool*retainsExistingPeers=nullptr){
  if(active.size()>=16||++files>256)throw std::runtime_error("material include file/depth extent");if(std::find(active.begin(),active.end(),path)!=active.end())throw std::runtime_error("material include cycle");auto source=provider(path);if(!source)throw std::runtime_error("material include source missing: "+path);if(source->size()>(1u<<24)||source->size()>(1u<<26)-bytes)throw std::runtime_error("material include byte extent");bytes+=source->size();dependencies.push_back(path);active.push_back(path);
  // Apply the already verified encoding/source-size gate before span scanning.
  const SourceMaterialKeyValuesParserCandidate checked(*source);(void)checked;TopLevel lexer(*source);SourceMaterialKeyValuesDocumentCandidate result;std::vector<SourceMaterialKeyValuesDocumentCandidate>includes,bases;
  for(;;){auto key=lexer.next();if(key.kind==Token::End)break;if(key.kind!=Token::Text||key.text.empty())throw std::runtime_error("material file root/directive required");auto lower=sourceMaterialPatchLowerCandidate(key.text);
   if(lower=="#include"||lower=="#base"){auto name=lexer.next();if(name.kind!=Token::Text||name.text.empty())throw std::runtime_error("material include filename required");if(name.text[0]=='/'||name.text[0]=='\\')throw std::runtime_error("absolute material include path");if(lower=="#include"&&!includes.empty())throw std::runtime_error("multiple includes per file require original lifetime clarification");auto slash=path.find_last_of('/');auto child=sourceMaterialKeyValuesPathCandidate((slash==std::string::npos?std::string():path.substr(0,slash+1))+name.text);auto loaded=load(child);if(lower=="#include")includes.push_back(std::move(loaded));else bases.push_back(std::move(loaded));continue;}
   auto open=lexer.next();if(open.kind!=Token::Open)throw std::runtime_error("material root block required");size_t depth=1,last=open.end;while(depth){auto token=lexer.next();if(token.kind==Token::End)throw std::runtime_error("material file truncated block");if(token.kind==Token::Open&&++depth>18)throw std::runtime_error("material root nesting extent");if(token.kind==Token::Close)--depth;last=token.end;}auto parsed=sourceMaterialKeyValuesParseCandidate(source->substr(key.begin,last-key.begin),rangeError);rangeError=parsed.numericRangeError;if(parsed.roots.size()!=1)throw std::runtime_error("material root span ambiguity");nodes+=count(parsed.roots[0]);if(nodes>65536)throw std::runtime_error("material include total node extent");result.roots.push_back(std::move(parsed.roots[0]));
  }
  if(result.roots.empty())throw std::runtime_error("material file local root required");if(retainsExistingPeers)*retainsExistingPeers=result.roots.size()==1;for(auto&included:includes)for(auto&root:included.roots)result.roots.push_back(std::move(root));for(const auto&base:bases)mergeBase(result.roots[0],base.roots[0]);size_t total=0;for(const auto&root:result.roots){total+=count(root);if(total>65536)throw std::runtime_error("resolved document node extent");}result.numericRangeError=rangeError;active.pop_back();return result;
 }
public:
 explicit SourceMaterialKeyValuesFilesCandidate(SourceMaterialKeyValuesSourceCandidate source):provider(std::move(source)){if(!provider)throw std::runtime_error("material source provider required");}
 SourceMaterialKeyValuesFilesResultCandidate resolve(const std::string&path,bool initialRangeError=false){active.clear();dependencies.clear();bytes=files=nodes=0;rangeError=initialRangeError;SourceMaterialKeyValuesFilesResultCandidate result;result.document=load(sourceMaterialKeyValuesPathCandidate(path),&result.retainsExistingPeers);result.dependencies=dependencies;return result;}
};
inline SourceMaterialKeyValuesFilesResultCandidate sourceMaterialKeyValuesFilesCandidate(const std::string&path,const SourceMaterialKeyValuesSourceCandidate&provider,bool initialRangeError=false){return SourceMaterialKeyValuesFilesCandidate(provider).resolve(path,initialRangeError);}
