#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
#include "expression_parser.h"
#include <iomanip>
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: inspect_session_expressions session.dmx");
 auto graph=loadGraph(argv[1]);size_t operators=0,parsed=0,complete=0;
 for(size_t index=0;index<graph.elements.size();++index){const auto&element=graph.elements[index];if(graph.strings[element.type]!="DmeExpressionOperator")continue;++operators;
  ExpressionVariables variables;const Attribute*formula=nullptr;const Attribute*result=nullptr;
  for(const auto&attribute:element.attributes){const auto&name=graph.strings[attribute.name];
   if(name=="expr")formula=&attribute;
   if(name=="result")result=&attribute;
   else if(attribute.type==3&&attribute.values.size()==1)variables.set(name,wireFloat(attribute.values[0]));
  }
  if(!formula||formula->type!=5||formula->values.size()!=1)throw std::runtime_error("invalid expression attribute schema");
  const auto&text=formula->values[0].text;ExpressionParser parser(text,variables);float value=0;bool success=parser.evaluate(value);parsed+=success;
  size_t end=parser.consumed();while(end<text.size()&&(text[end]==' '||text[end]=='\t'))++end;bool entire=success&&end==text.size();complete+=entire;
  std::cout<<index<<" name="<<std::quoted(graph.strings[element.name])<<" expr="<<std::quoted(text)<<" variables="<<variables.size()<<" parsed="<<success<<" complete="<<entire<<" consumed="<<parser.consumed()<<" result="<<std::setprecision(9)<<value;
  if(result&&result->type==3&&result->values.size()==1)std::cout<<" stored="<<wireFloat(result->values[0]);std::cout<<'\n';
 }
 std::cout<<"operators="<<operators<<" parsed="<<parsed<<" complete="<<complete<<'\n';
 return 0;
 }catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
