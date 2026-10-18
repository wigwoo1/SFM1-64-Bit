#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("usage: test_graph_edits fixture new-output");
 auto g=loadGraph(argv[1]);g.elements.at(0).name=internString(g,"native_x64_edit_validation");
 size_t ei=0,ai=0;bool found=false;
 for(size_t i=0;i<g.elements.size()&&!found;++i)for(size_t j=0;j<g.elements[i].attributes.size();++j){auto&a=g.elements[i].attributes[j];if(a.type==5){ei=i;ai=j;found=true;break;}}
 if(!found)throw std::runtime_error("fixture lacks string attribute");
 size_t fi=0,fa=0;bool floatFound=false;
 for(size_t i=0;i<g.elements.size()&&!floatFound;++i)for(size_t j=0;j<g.elements[i].attributes.size();++j)if(g.elements[i].attributes[j].type==3){fi=i;fa=j;floatFound=true;break;}
 if(!floatFound)throw std::runtime_error("fixture lacks float attribute");
 setFloat(g.elements[fi].attributes[fa],0,-1.25f);
 Value integer;integer.wire.resize(4);setWireU32(integer,0xffffffff);if(wireInt(integer)!=-1)throw std::runtime_error("signed integer decoding failed");
 setString(g,g.elements[ei].attributes[ai],0,"native_x64_string_edit_validation");saveGraph(g,argv[2]);auto read=loadGraph(argv[2]);validateGraph(read);
 if(read.strings[read.elements[0].name]!="native_x64_edit_validation"||read.elements[ei].attributes[ai].values[0].text!="native_x64_string_edit_validation")throw std::runtime_error("edit failed to persist");
 if(wireFloat(read.elements[fi].attributes[fa].values[0])!=-1.25f)throw std::runtime_error("numeric edit failed to persist");
 auto bad=read;bad.elements[0].name=UINT32_MAX;bool rejected=false;try{validateGraph(bad);}catch(const std::exception&){rejected=true;}if(!rejected)throw std::runtime_error("invalid index accepted");
 std::cout<<"{\"rename_persisted\":true,\"string_edit_persisted\":true,\"float_edit_persisted\":true,\"signed_int_decoded\":true,\"invalid_index_rejected\":true}\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
