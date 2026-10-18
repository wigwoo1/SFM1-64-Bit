from pathlib import Path
p=Path(__file__).with_name('sfm64_session_core.cpp')
s=p.read_text()
assert '#include "session_fresh_flex_pass.h"' not in s
s=s.replace('#include <charconv>','#include "session_fresh_flex_pass.h"\n#include <charconv>',1)
marker='if(argc>=3&&std::string(argv[1])=="list-expression-chains")'
assert s.count(marker)==1
branch='''if(argc>=2&&std::string(argv[1])=="evaluate-fresh-flex"){
 if(argc!=5)throw std::runtime_error("usage: evaluate-fresh-flex INPUT FRESH_OUTPUT CONTENT_ROOT");
 auto graph=loadGraph(argv[2]);validateGraph(graph);
 RecoveredOwnedDmxReferenceProjectionCandidate<Graph> session(std::move(graph),[](const std::string&,auto&)->uint32_t{throw std::runtime_error("external references require resolver");});
 auto result=executeSessionFreshFlexPassCandidate(session,argv[4]);
 auto exported=session.exportReferences([](uint32_t,const auto&)->std::string{throw std::runtime_error("external output requires resolver");});saveGraph(exported,argv[3]);
 session.scene.cleanup({},[](auto&,bool){},[](auto&){});
 if(session.scene.size()||session.scene.attributes.size()||session.scene.references.nodes.allocationCount())throw std::runtime_error("fresh flex teardown failed");
 std::cout<<"isolated_fresh_flex bindings="<<result.bindings<<" registry_names="<<result.names<<" changed="<<result.changed<<" equal="<<result.equal<<" historical_mapping=unverified timeline=not_evaluated native_scheduler=unfinished\\n"<<"saved="<<argv[3]<<'\\n';return 0;
 }'''
s=s.replace(marker,branch+marker,1)
p.write_text(s)
