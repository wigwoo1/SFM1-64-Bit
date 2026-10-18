#pragma once
#include "projected_light_local_matrix_candidate.h"
#include "matrix_compose_candidate.h"
// Graph must be immutable for this evaluator's lifetime. Saved ordinary DAG only;
// native type inheritance, override policy and animated evaluation remain separate.
class SavedDagWorldMatrixCandidate {
    const Graph& graph;
    std::vector<std::vector<size_t>> owners;
    static std::array<uint32_t,12> identity(){return {{0x3f800000,0,0,0,0,0x3f800000,0,0,0,0,0x3f800000,0}};}
    std::array<uint32_t,12> local(size_t index)const{
        const auto& element=graph.elements.at(index);
        const auto& type=graph.strings.at(element.type);
        if(type!="DmeDag"&&type!="DmeProjectedLight")throw std::invalid_argument("unverified DAG class");
        const Attribute* reference=nullptr;
        for(const auto& a:element.attributes){const auto& name=graph.strings.at(a.name);
            if(name=="overridePos"||name=="overrideRot"||name=="overrideParent")throw std::invalid_argument("DAG override policy unverified");
            if(name=="transform"){if(reference)throw std::invalid_argument("duplicate DAG transform");reference=&a;}
        }
        if(!reference||reference->type!=1||reference->values.size()!=1)throw std::invalid_argument("DAG transform shape");
        const auto ref=reference->values[0].reference;
        if(ref==0xffffffffu)return identity();
        if(ref>=graph.elements.size())throw std::invalid_argument("DAG transform reference");
        const auto& transform=graph.elements[ref];
        if(graph.strings.at(transform.type)!="DmeTransform")throw std::invalid_argument("DAG transform class");
        ProjectedLightSavedFieldsCandidate input;
        bool position=false,orientation=false;
        for(const auto& a:transform.attributes){const auto& name=graph.strings.at(a.name);
            if(name=="position"||name=="orientation"){
                const bool isPosition=name=="position";auto& present=isPosition?position:orientation;
                const size_t bytes=isPosition?12:16;
                if(present||a.type!=(isPosition?10:13)||a.values.size()!=1||a.values[0].wire.size()!=bytes)throw std::invalid_argument("DAG transform vector shape");
                present=true;std::memcpy(isPosition?static_cast<void*>(input.localPositionWords.data()):static_cast<void*>(input.localOrientationWords.data()),a.values[0].wire.data(),bytes);
            }else if(name=="scale"){
                if(input.hasLocalScale||a.type!=3||a.values.size()!=1||a.values[0].wire.size()!=4)throw std::invalid_argument("DAG scale shape");
                input.hasLocalScale=true;input.localScaleWord=wireU32(a.values[0]);
            }
        }
        if(!position||!orientation)throw std::invalid_argument("DAG transform values missing");
        return projectedLightLocalMatrixCandidate(input);
    }
public:
    explicit SavedDagWorldMatrixCandidate(const Graph& source):graph(source),owners(source.elements.size()){
        for(size_t owner=0;owner<graph.elements.size();++owner)for(const auto& a:graph.elements[owner].attributes)if(graph.strings.at(a.name)=="children"){
            if(a.type!=15)throw std::invalid_argument("DAG children shape");
            for(const auto& value:a.values){if(value.reference==0xffffffffu)continue;
                if(value.reference>=owners.size())throw std::invalid_argument("DAG child reference");
                auto& list=owners[value.reference];if(std::find(list.begin(),list.end(),owner)==list.end())list.push_back(owner);
            }
        }
    }
    std::array<uint32_t,12> evaluate(size_t index)const{
        std::vector<size_t> chain;
        for(;;){
            if(index>=owners.size())throw std::invalid_argument("DAG index");
            if(chain.size()>=4096||std::find(chain.begin(),chain.end(),index)!=chain.end())throw std::invalid_argument("DAG depth or cycle");
            chain.push_back(index);
            if(owners[index].empty())break;
            if(owners[index].size()!=1)throw std::invalid_argument("ambiguous DAG parent");
            index=owners[index][0];
        }
        auto result=identity();
        for(auto it=chain.rbegin();it!=chain.rend();++it)result=matrixComposeCandidate(result,local(*it));
        return result;
    }
};
