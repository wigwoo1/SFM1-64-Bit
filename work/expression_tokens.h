#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <utility>
// Matches the recovered default/C-locale path; alternate locale pending.
inline unsigned char expressionFold(unsigned char c){return c>='A'&&c<='Z'?static_cast<unsigned char>(c+32):c;}
inline bool expressionToken(std::string_view text,size_t&cursor,std::string_view token){
 if(cursor>text.size())return false;
 size_t next=cursor;
 while(next<text.size()&&(text[next]==' '||text[next]=='\t'))++next;
 if(token.size()>text.size()-next)return false;
 for(size_t i=0;i<token.size();++i)if(expressionFold(static_cast<unsigned char>(text[next+i]))!=expressionFold(static_cast<unsigned char>(token[i])))return false;
 cursor=next+token.size();return true;
}
class ExpressionVariables{
 std::vector<std::pair<std::string,float>> variables;
public:
 void set(std::string name,float value){for(auto&entry:variables)if(entry.first==name){entry.second=value;return;}variables.emplace_back(std::move(name),value);}
 size_t size()const{return variables.size();}
 bool match(std::string_view text,size_t&cursor,float&value)const{
  for(auto i=variables.rbegin();i!=variables.rend();++i)if(expressionToken(text,cursor,i->first)){value=i->second;return true;}
  return false;
 }
};
