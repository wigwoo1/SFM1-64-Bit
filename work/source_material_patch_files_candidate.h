#pragma once
#include "source_material_keyvalues_files_candidate.h"
#include <charconv>
#include <set>
// Original10006BA0 file/patch resolution before shader defaults, fallbacks,
// parameter extraction or rendering. GAME logical source provider only.
enum class SourceMaterialPatchFileLoadStatusCandidate{Loaded,RootUnavailable,RetainRootName};
struct SourceMaterialPatchFilesResultCandidate {
 SourceMaterialPatchFileLoadStatusCandidate status=SourceMaterialPatchFileLoadStatusCandidate::Loaded;
 uint32_t diagnosticCalls=0,developerDiagnosticCalls=0;
 SourceMaterialKeyValuesDocumentCandidate document;
 SourceMaterialPatchNodeCandidate aggregate;
 std::vector<std::string>dependencies,patchFiles;
};
inline void sourceMaterialPatchFileTreeValidateCandidate(const SourceMaterialPatchNodeCandidate&n,size_t depth,size_t&count){
 if(depth>17||++count>65536||(n.kind>4&&n.kind!=7)||n.key.size()>32767||n.text.size()>32767)throw std::runtime_error("material file patch tree extent/type");size_t segment=0,segments=1;for(unsigned char c:n.key){if(c<32||c>126)throw std::runtime_error("material file patch key encoding");if(c=='/'){segment=0;if(++segments>16)throw std::runtime_error("material file patch key path depth");}else if(++segment>255)throw std::runtime_error("material file patch key segment extent");}for(unsigned char c:n.text)if(c>126||(c<32&&c!='\t'&&c!='\r'&&c!='\n'&&c!='\f'&&c!='\v'))throw std::runtime_error("material file patch text encoding");if(n.kind==3){float value;std::memcpy(&value,&n.bits,4);if(std::isnan(value))throw std::runtime_error("material file patch NaN unsupported");}for(const auto&child:n.children)sourceMaterialPatchFileTreeValidateCandidate(child,depth+1,count);
}
inline void sourceMaterialPatchFileCopyCandidate(SourceMaterialPatchNodeCandidate&target,const SourceMaterialPatchNodeCandidate&source){
 // Original10004FE0 performs separate value and true-block passes, using
 // typed setters/FindKey. This is distinct from the final apply helper4DE0.
 for(const auto&child:source.children)if(child.kind>=1&&child.kind<=4){auto*to=sourceMaterialPatchFindCandidate(target,child.key,true);to->kind=child.kind;to->text=child.text;to->bits=child.bits;to->pointer=child.pointer;}
 for(const auto&child:source.children)if(child.kind==0)sourceMaterialPatchFileCopyCandidate(*sourceMaterialPatchFindCandidate(target,child.key,true),child);
}
inline void sourceMaterialPatchFileAccumulateCandidate(SourceMaterialPatchNodeCandidate&target,const SourceMaterialPatchNodeCandidate&source){
 for(const std::string name:{"insert","replace"}){auto*section=sourceMaterialPatchFindCandidate(target,name,false);if(!section){SourceMaterialPatchNodeCandidate node;node.key=name;target.children.push_back(std::move(node));section=&target.children.back();}const auto*from=sourceMaterialPatchFindConstCandidate(source,name);if(from)sourceMaterialPatchFileCopyCandidate(*section,*from);}
}
inline std::string sourceMaterialPatchIncludeNameCandidate(const SourceMaterialPatchNodeCandidate&node){
 if(node.kind==1)return node.text;if(node.kind==2){int32_t v;std::memcpy(&v,&node.bits,4);return std::to_string(v);}if(node.kind==7){int64_t v;std::memcpy(&v,&node.pointer,8);return std::to_string(v);}if(node.kind==3){float value;std::memcpy(&value,&node.bits,4);if(!std::isfinite(value))throw std::runtime_error("nonfinite material include conversion unsupported");double number=value;if(std::abs(number)>=1e17){/* Original CRT fixed formatting caps at17 significant digits and fills larger integer places with zeros. */char scientific[64];auto formatted=std::to_chars(scientific,scientific+sizeof(scientific),std::abs(number),std::chars_format::scientific,16);if(formatted.ec!=std::errc{})throw std::runtime_error("material include scientific format extent");std::string digits;char*exponent=nullptr;for(char*p=scientific;p<formatted.ptr;++p){if(*p=='e'){exponent=p+1;break;}if(*p!='.')digits+=*p;}if(!exponent)throw std::runtime_error("material include scientific exponent");if(*exponent=='+')++exponent;int power=0;auto parsed=std::from_chars(exponent,formatted.ptr,power);if(parsed.ec!=std::errc{}||parsed.ptr!=formatted.ptr||power<17||power>38||digits.size()!=17)throw std::runtime_error("material include scientific bounds");digits.append(size_t(power+1)-digits.size(),'0');return (std::signbit(number)?std::string("-"):std::string())+digits+".000000";}const double scaled=std::abs(number)*1000000.0;if(std::fmod(scaled,1.0)==0.5)number=std::nextafter(number,std::signbit(number)?-std::numeric_limits<double>::infinity():std::numeric_limits<double>::infinity());char output[64];auto result=std::to_chars(output,output+sizeof(output),number,std::chars_format::fixed,6);if(result.ec!=std::errc{})throw std::runtime_error("material include float format extent");return std::string(output,result.ptr);}throw std::runtime_error("unsupported material include value type");
}
inline void sourceMaterialPatchFileAssignedTreeCandidate(SourceMaterialPatchNodeCandidate&node){
 // Original70E50 assigns a scalar type/value only for leaves. Child-bearing
 // nodes are rebuilt through fresh block constructors, even if source hybrid.
 if(!node.children.empty()){node.kind=0;node.text.clear();node.bits=0;node.pointer=0;}for(auto&child:node.children)sourceMaterialPatchFileAssignedTreeCandidate(child);
}
inline SourceMaterialPatchFilesResultCandidate sourceMaterialPatchFilesCandidate(const std::string&materialName,const SourceMaterialKeyValuesSourceCandidate&provider,bool prefixAlreadyProvided=false,bool recoverPartial=false){
 if(!provider||materialName.empty()||materialName[0]=='{')throw std::runtime_error("material file name/source scope");const auto requested=(prefixAlreadyProvided?std::string():std::string("materials/"))+materialName+".vmt";if(requested.size()>=260)throw std::runtime_error("material file root path extent");const auto rootPath=sourceMaterialKeyValuesPathCandidate(requested);SourceMaterialPatchFilesResultCandidate out;out.aggregate.key="patchAccumulator";size_t bytes=0,reads=0;SourceMaterialKeyValuesSourceCandidate checked=[&](const std::string&path)->std::optional<std::string>{if(++reads>256)throw std::runtime_error("material patch source read extent");auto value=provider(path);if(!value)return {};if(value->size()>(1u<<26)-bytes)throw std::runtime_error("material patch source total byte extent");bytes+=value->size();out.dependencies.push_back(path);return value;};
 // Partial-load mode distinguishes unavailable root files from nested patch
 // failure. Unsupported grammar/provider errors still throw transactionally.
 auto load=[&](const std::string&path,bool range)->std::optional<SourceMaterialKeyValuesFilesResultCandidate>{
  if(!recoverPartial)return sourceMaterialKeyValuesFilesCandidate(path,checked,range);
  auto first=checked(path);if(!first||first->empty())return {};
  bool pending=true;auto cached=[&](const std::string&p)->std::optional<std::string>{if(pending&&p==path){pending=false;return std::move(first);}return checked(p);};
  return sourceMaterialKeyValuesFilesCandidate(path,cached,range,true);
 };
 auto initial=load(rootPath,false);if(!initial){out.status=SourceMaterialPatchFileLoadStatusCandidate::RootUnavailable;return out;}
 auto current=std::move(*initial);out.diagnosticCalls=current.document.diagnosticCalls;out.developerDiagnosticCalls=current.document.developerDiagnosticCalls;
 auto validate=[&](const SourceMaterialKeyValuesDocumentCandidate&doc){size_t count=0;for(const auto&node:doc.roots)sourceMaterialPatchFileTreeValidateCandidate(node,0,count);};
 validate(current.document);if(current.document.retainsRootName){out.status=SourceMaterialPatchFileLoadStatusCandidate::RetainRootName;out.document=std::move(current.document);return out;}if(sourceMaterialPatchLowerCandidate(current.document.roots[0].key)!="patch"){out.document=std::move(current.document);return out;}
 const auto originalDocument=current.document;
 auto failedPatch=[&](){++out.diagnosticCalls;out.document=originalDocument;return out;};
 // Original MakeCopy drops initial peers; reused LoadFromBuffer retains older
 // peers on later single-local-root loads, before appending #include peers.
 current.document.roots.resize(1);std::set<std::string>seen;seen.insert(rootPath);bool rangeError=current.document.numericRangeError;std::string currentPath=rootPath;
 while(sourceMaterialPatchLowerCandidate(current.document.roots[0].key)=="patch"){
  if(out.patchFiles.size()>=10)throw std::runtime_error("material patch chain remains unresolved at native limit");out.patchFiles.push_back(currentPath);const auto&patch=current.document.roots[0];sourceMaterialPatchFileAccumulateCandidate(out.aggregate,patch);size_t count=0;sourceMaterialPatchFileTreeValidateCandidate(out.aggregate,0,count);const auto*include=sourceMaterialPatchFindConstCandidate(patch,"include");if(!include){if(recoverPartial)return failedPatch();throw std::runtime_error("material patch include missing");}const auto name=sourceMaterialPatchIncludeNameCandidate(*include);const auto nextPath=sourceMaterialKeyValuesPathCandidate(name);if(!seen.insert(nextPath).second)throw std::runtime_error("material patch include cycle");auto loaded=load(nextPath,rangeError);if(!loaded)return failedPatch();auto next=std::move(*loaded);if(next.document.retainsRootName)throw std::runtime_error("retained-name patch include unverified");out.diagnosticCalls+=next.document.diagnosticCalls;out.developerDiagnosticCalls+=next.document.developerDiagnosticCalls;rangeError=next.document.numericRangeError;if(next.retainsExistingPeers&&current.document.roots.size()>1)next.document.roots.insert(next.document.roots.begin()+1,current.document.roots.begin()+1,current.document.roots.end());validate(next.document);current=std::move(next);currentPath=nextPath;
 }
 // Aggregate contains only typed0..4 writes. Preserve untouched uint64 and
 // infinity in the raw base instead of passing it through the older restricted
 // patch-only public validator. The native final copy helper skips type7 writes.
 auto&base=current.document.roots[0];if(const auto*insert=sourceMaterialPatchFindConstCandidate(out.aggregate,"insert"))sourceMaterialPatchCopyCandidate(base,*insert,false,false);if(const auto*replace=sourceMaterialPatchFindConstCandidate(out.aggregate,"replace"))sourceMaterialPatchCopyCandidate(base,*replace,true,false);for(auto&node:current.document.roots)sourceMaterialPatchFileAssignedTreeCandidate(node);validate(current.document);out.document=std::move(current.document);return out;
}
