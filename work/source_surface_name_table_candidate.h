#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
struct SourceSurfaceScriptCandidate {std::string name,text;};
// Name/index portion of VPhysicsSurfaceProps001 only. Physical/audio values
// are deliberately not represented; this table is sufficient for studio trace
// surface IDs, not a replacement physics/material database.
class SourceSurfaceNameTableCandidate {
    std::vector<std::string>names_;
    std::unordered_map<std::string,int32_t>indices_;
    std::vector<int32_t>parseCounts_;
    static std::string lower(std::string name){for(char&c:name){const auto u=static_cast<unsigned char>(c);if(u<32||u>126)throw std::runtime_error("surface names require checked ASCII");if(c>='A'&&c<='Z')c=char(c+32);}return name;}
    struct Token {enum Kind {End,Text,Open,Close}kind=End;std::string text;};
    class Tokens {
        const std::string&text_;size_t pos_=0;
    public:
        explicit Tokens(const std::string&text):text_(text){for(unsigned char c:text)if((c<32&&c!='\t'&&c!='\r'&&c!='\n')||c>126)throw std::runtime_error("unsupported surface script byte");}
        Token next(){
            for(;;){while(pos_<text_.size()&&(text_[pos_]==' '||text_[pos_]=='\t'||text_[pos_]=='\r'||text_[pos_]=='\n'))++pos_;if(pos_+1<text_.size()&&text_[pos_]=='/'&&text_[pos_+1]=='/'){while(pos_<text_.size()&&text_[pos_]!='\n')++pos_;continue;}break;}
            if(pos_==text_.size())return {};
            const char first=text_[pos_++];if(first=='{')return {Token::Open,{}};if(first=='}')return {Token::Close,{}};
            if(first=='#'||first=='['||first==']'||first=='\\'||first=='('||first==')'||first==':'||first=='\''||(first=='/'&&pos_<text_.size()&&text_[pos_]=='*'))throw std::runtime_error("unsupported surface script token");
            std::string value;
            if(first=='"'){bool closed=false;while(pos_<text_.size()){char c=text_[pos_++];if(c=='"'){closed=true;break;}if(c=='\\'||c=='\r'||c=='\n'||c=='{'||c=='}')throw std::runtime_error("surface script escape/multiline/brace string unsupported");value+=c;}if(!closed)throw std::runtime_error("unterminated surface script quote");}
            else{value+=first;while(pos_<text_.size()){char c=text_[pos_];if(c==' '||c=='\t'||c=='\r'||c=='\n'||c=='{'||c=='}')break;if(c=='"'||c=='\\'||c=='['||c==']'||c=='#'||c=='('||c==')'||c==':'||c=='\''||(c=='/'&&pos_+1<text_.size()&&text_[pos_+1]=='*'))throw std::runtime_error("unsupported surface script token");value+=c;++pos_;}}
            if(value.size()>1023)throw std::runtime_error("surface script token exceeds original buffer");return {Token::Text,std::move(value)};
        }
    };
public:
    explicit SourceSurfaceNameTableCandidate(const std::vector<SourceSurfaceScriptCandidate>&scripts){
        std::unordered_set<std::string>parsedFiles;bool shadowAdded=false;
        for(const auto&script:scripts){
            if(script.name.empty())throw std::runtime_error("surface script identity required");
            if(!parsedFiles.insert(lower(script.name)).second){parseCounts_.push_back(0);continue;}
            Tokens tokens(script.text);
            for(;;){auto name=tokens.next();if(name.kind==Token::End)break;if(name.kind!=Token::Text||name.text.empty()||tokens.next().kind!=Token::Open)throw std::runtime_error("surface declaration requires name/block");
                // Checked flat key/value grammar. Includes/conditions/nested
                // blocks must not silently contribute fabricated declarations.
                for(;;){const auto key=tokens.next();if(key.kind==Token::Close)break;if(key.kind!=Token::Text||tokens.next().kind!=Token::Text)throw std::runtime_error("surface declaration requires flat key/value pairs");}
                const auto key=lower(name.text);if(key=="$material_index_shadow")throw std::runtime_error("reserved surface declaration unsupported");if(!indices_.count(key)){if(names_.size()>=65535)throw std::runtime_error("surface index domain exhausted");indices_.emplace(key,int32_t(names_.size()));names_.push_back(std::move(name.text));}
            }
            // Original parser appends one reserved shadow record after its
            // FIRST unique script. It shifts subsequent normal indices.
            if(!shadowAdded){if(names_.size()>=65535)throw std::runtime_error("surface index domain exhausted");shadowAdded=true;names_.push_back("$MATERIAL_INDEX_SHADOW");}
            parseCounts_.push_back(int32_t(names_.size()));
        }
    }
    int32_t lookup(const std::string&name)const{const auto key=lower(name);if(key=="$material_index_shadow")return 0xf000;const auto found=indices_.find(key);return found==indices_.end()?-1:found->second;}
    size_t size()const{return names_.size();}
    const auto&parseCounts()const{return parseCounts_;}
};
