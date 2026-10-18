#pragma once
// Requires Graph, namedAttribute and hierarchyRefs declarations.
struct ChannelEndpoint {uint32_t element=0xffffffff;std::string attribute;int32_t index=0;uint8_t scalarType=0;bool present()const{return element!=0xffffffff;}};
inline ChannelEndpoint resolveEndpoint(const Graph&g,const Element&e,const char*side){
 ChannelEndpoint b;std::string prefix=side;auto ref=hierarchySingleReference(g,e,(prefix+"Element").c_str());auto*a=namedAttribute(g,e,(prefix+"Attribute").c_str());auto*i=namedAttribute(g,e,(prefix+"Index").c_str());
 if(!a||a->type!=5||a->values.size()!=1||!i||i->type!=2||i->values.size()!=1)throw std::runtime_error("invalid endpoint metadata");b.attribute=a->values[0].text;b.index=wireInt(i->values[0]);if(ref==0xffffffff)return b;
 b.element=ref;auto*target=namedAttribute(g,g.elements.at(b.element),b.attribute.c_str());if(!target)throw std::runtime_error("missing endpoint attribute");
 bool array=target->type>=15;b.scalarType=array?target->type-14:target->type;
 if(b.index<0||(!array&&b.index!=0)||static_cast<size_t>(b.index)>=target->values.size())throw std::runtime_error("endpoint index out of range");return b;
}
struct ChannelBinding {
 ChannelEndpoint source,destination;int32_t mode=0;uint32_t log=0xffffffff;std::vector<uint32_t> layers;
 ChannelBinding(const Graph&g,uint32_t channel){auto&e=g.elements.at(channel);if(g.strings.at(e.type)!="DmeChannel")throw std::runtime_error("expected channel");source=resolveEndpoint(g,e,"from");destination=resolveEndpoint(g,e,"to");
  auto*m=namedAttribute(g,e,"mode");if(!m||m->type!=2||m->values.size()!=1)throw std::runtime_error("invalid channel mode");mode=wireInt(m->values[0]);auto logs=hierarchyRefs(g,e,"log",1);if(logs.empty())return;log=logs.at(0);auto&le=g.elements.at(log);layers=hierarchyRefs(g,le,"layers",15);
  auto*d=namedAttribute(g,le,"defaultvalue");if(!d||d->values.size()!=1)throw std::runtime_error("invalid log default");if(destination.present()&&d->type!=destination.scalarType)throw std::runtime_error("log/destination type mismatch");
  for(auto ref:layers){auto*v=namedAttribute(g,g.elements.at(ref),"values");if(!v||v->type!=d->type+14)throw std::runtime_error("log layer type mismatch");}
 }
};
