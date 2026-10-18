#pragma once
struct FloatLogDefault {
 bool enabled=false;float value=0;
 FloatLogDefault(const Graph&g,const Element&log){
  if(g.strings.at(log.type)!="DmeFloatLog")throw std::runtime_error("expected float log");auto*u=namedAttribute(g,log,"usedefaultvalue");auto*d=namedAttribute(g,log,"defaultvalue");
  if(!u||u->type!=4||u->values.size()!=1||u->values[0].wire.size()!=1||!d||d->type!=3||d->values.size()!=1)throw std::runtime_error("invalid float log default");enabled=u->values[0].wire[0]!=0;value=wireFloat(d->values[0]);
 }
 float emptyLayerValue()const{return enabled?value:0.0f;}
};
