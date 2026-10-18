#define DMX_GRAPH_NO_MAIN
#include "dmx_graph.cpp"
int main(int argc,char**argv){try{
 if(argc!=2)throw std::runtime_error("usage: inspect_animation session.dmx");auto g=loadGraph(argv[1]);std::map<std::string,uint64_t> classes;uint64_t frames=0,layers=0;
 for(auto&e:g.elements){auto&t=g.strings[e.type];++classes[t];if(t=="DmeTimeFrame")++frames;if(t.find("LogLayer")!=std::string::npos)++layers;}
 std::cout<<"time_frames="<<frames<<" log_layers="<<layers<<'\n';unsigned shown=0;
 for(auto&e:g.elements)if(g.strings[e.type]=="DmeTimeFrame"&&shown++<3){for(auto&a:e.attributes){std::cout<<g.strings[a.name]<<" type="<<unsigned(a.type);if((a.type==7||a.type==2)&&a.values.size()==1)std::cout<<" raw_int="<<wireInt(a.values[0]);if(a.type==3&&a.values.size()==1)std::cout<<" float="<<wireFloat(a.values[0]);std::cout<<'\n';}}
 for(auto&entry:classes)if(entry.first.find("Log")!=std::string::npos)std::cout<<entry.first<<'='<<entry.second<<'\n';
 std::map<std::string,bool> shownTypes;uint64_t matched=0,mismatched=0,unsorted=0,totalKeys=0;
 for(auto&e:g.elements){auto&t=g.strings[e.type];if(t.find("LogLayer")==std::string::npos)continue;const Attribute*times=nullptr,*values=nullptr;
  for(auto&a:e.attributes){if(g.strings[a.name]=="times")times=&a;if(g.strings[a.name]=="values")values=&a;}
  if(times&&values){if(times->values.size()==values->values.size())++matched;else ++mismatched;totalKeys+=times->values.size();for(size_t i=1;i<times->values.size();++i)if(wireInt(times->values[i])<wireInt(times->values[i-1])){++unsorted;break;}}
  if(!shownTypes[t]){shownTypes[t]=true;std::cout<<"schema "<<t<<'\n';for(auto&a:e.attributes){std::cout<<g.strings[a.name]<<" type="<<unsigned(a.type)<<" count="<<a.values.size();if(a.type==2&&a.values.size()==1)std::cout<<" int="<<wireInt(a.values[0]);std::cout<<'\n';}}
 }
 std::cout<<"matched_key_arrays="<<matched<<" mismatched="<<mismatched<<" unsorted="<<unsorted<<" total_keys="<<totalKeys<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
