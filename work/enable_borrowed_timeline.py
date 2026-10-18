from pathlib import Path
root=Path(__file__).parent
p=root/'test_prepared_timeline_pass_x64.cpp';s=p.read_text()
old='SessionRuntime retained(base),reference(base);PreparedSessionExpressionChainPassCandidate'
assert s.count(old)==1
new='''
#ifdef TEST_BORROWED_TIMELINE
 RecoveredOwnedDmxReferenceProjectionCandidate<Graph> projection(base,[](const std::string&,auto&)->uint32_t{throw std::runtime_error("external borrowed graph");});
 SessionRuntime retained(projection.graph,BorrowedSessionGraphCandidate{}),reference(base);
 if(retained.ownedGraph||&retained.graph!=&projection.graph)throw std::runtime_error("borrowed graph copied");
#else
 SessionRuntime retained(base),reference(base);
#endif
 PreparedSessionExpressionChainPassCandidate'''
s=s.replace(old,new,1)
marker=' std::cout<<"prepared timeline frames="'
assert s.count(marker)==1
checks='''
#ifdef TEST_BORROWED_TIMELINE
 auto protectedGraph=projection.graph;unsigned rejectedEdits=0;
 try{retained.setReference(retained.elements.graphHandles.at(0),"activeClip",0,0xffffffffu);}catch(const std::runtime_error&){++rejectedEdits;}
 try{retained.setReferenceRange(retained.elements.graphHandles.at(0),"activeClip",0,{0xffffffffu},false,[](int,uint32_t,size_t,size_t){});}catch(const std::runtime_error&){++rejectedEdits;}
 try{retained.removeAttribute(retained.elements.graphHandles.at(0),"activeClip");}catch(const std::runtime_error&){++rejectedEdits;}
 if(rejectedEdits!=3||!graphExactEqualCandidate(protectedGraph,projection.graph))throw std::runtime_error("borrowed owner mutation bypass");
 auto exported=projection.exportReferences([](uint32_t,const auto&)->std::string{throw std::runtime_error("external export");});if(!graphExactEqualCandidate(exported,projection.graph))throw std::runtime_error("borrowed owned export");projection.scene.cleanup({},[](auto&,bool){},[](auto&){});if(projection.scene.size()||projection.scene.references.nodes.allocationCount())throw std::runtime_error("borrowed owned teardown");
 std::cout<<"borrowed_graph_identity_owner_edit_guards_export_teardown=passed\\n";
#endif
'''
s=s.replace(marker,checks+marker,1);p.write_text(s)
p=root/'sfm64_session_core.cpp';s=p.read_text()
old=' SessionRuntime runtime(std::move(graph));auto typed=discoverSessionAdditionalPlayPathsCandidate(runtime.graph,root,discovery.chains);'
assert s.count(old)==1
new=' RecoveredOwnedDmxReferenceProjectionCandidate<Graph>session(std::move(graph),[](const std::string&,auto&)->uint32_t{throw std::runtime_error("external references require resolver");});SessionRuntime runtime(session.graph,BorrowedSessionGraphCandidate{});auto typed=discoverSessionAdditionalPlayPathsCandidate(runtime.graph,root,discovery.chains);'
s=s.replace(old,new,1)
old=' RecoveredOwnedDmxReferenceProjectionCandidate<Graph>session(std::move(runtime.graph),[](const std::string&,auto&)->uint32_t{throw std::runtime_error("external references require resolver");});auto exported='
assert s.count(old)==1;s=s.replace(old,' auto exported=',1);p.write_text(s)
