#pragma once
// Binary5 loader10395B60 publishes element-table[0] at10395F53..63.
// This applies to the supported binary session graph, not arbitrary formats.
inline uint32_t sessionDocumentRootCandidate(const Graph&graph){
 if(graph.header!="<!-- dmx encoding binary 5 format sfm_session 20 -->\n"&&
    graph.header!="<!-- dmx encoding binary 5 format sfm_session 20 -->")
  throw std::runtime_error("unsupported document root encoding");
 if(graph.elements.empty())throw std::runtime_error("session document root absent");
 return 0;
}
